// filemergerlogic.cpp

#include "filemergerlogic.h"
#include <QFile>
#include <QFileInfo> // For QFileInfo
#include <QTextStream>
#include <QDateTime>
#include <QThread>
#include <QDir>
#include <QDebug> // For logging
#include <QCoreApplication> // For tr

// --- MergeWorker Implementation ---
MergeWorker::MergeWorker(const QStringList &files, const QString &outputPath)
    : filesToMerge(files), outputPathBase(outputPath) {}

MergeWorker::~MergeWorker() {
    qDebug() << "MergeWorker destroyed";
}

void MergeWorker::process() {
    if (filesToMerge.isEmpty()) {
        emit finished(false, QCoreApplication::tr("没有选择文件进行合并。 (No files selected for merging.)"));
        emit progressUpdated(100); // Or 0, depending on convention for "nothing to do"
        return;
    }

    QString mergedContent;
    int fileCount = filesToMerge.count();
    int processedCount = 0;
    emit progressUpdated(0);

    for (const QString &filePath : filesToMerge) {
        if (QThread::currentThread()->isInterruptionRequested()) {
             emit finished(false, QCoreApplication::tr("合并操作已取消。(Merge operation cancelled.)"));
             emit progressUpdated(processedCount * 100 / fileCount); // Current progress before abort
             return;
        }

        QFile file(filePath);
        if (!file.exists()) {
            qWarning() << "File does not exist, skipping:" << filePath;
            // Optionally collect these errors and report them
            // For now, just skip and continue
            processedCount++;
            emit progressUpdated((processedCount * 100) / fileCount);
            continue;
        }
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit finished(false, QCoreApplication::tr("无法读取文件: (Could not read file:) ") + filePath + "\n" + file.errorString());
            emit progressUpdated((processedCount * 100) / fileCount);
            return;
        }

        QFileInfo fileInfo(filePath);
        mergedContent.append(QString("\n\n========== [%1] ==========\n\n").arg(fileInfo.fileName()));

        QTextStream in(&file);
        mergedContent.append(in.readAll());
        file.close();

        processedCount++;
        emit progressUpdated((processedCount * 100) / fileCount);
    }

    if (QThread::currentThread()->isInterruptionRequested()) {
         emit finished(false, QCoreApplication::tr("合并操作已取消。(Merge operation cancelled before saving.)"));
         return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString outputFilename = QString("collated_files_%1.txt").arg(timestamp);
    QString outputFilePath = QDir(outputPathBase).filePath(outputFilename);

    QFile outputFile(outputFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit finished(false, QCoreApplication::tr("无法创建输出文件: (Could not create output file:) ") + outputFilePath + "\n" + outputFile.errorString());
        emit progressUpdated(100); // Indicate process attempted completion
        return;
    }

    QTextStream out(&outputFile);
    out << mergedContent;
    outputFile.close();

    emit progressUpdated(100);
    emit finished(true, outputFilePath);
}


// --- FileMergerLogic Implementation ---
FileMergerLogic::FileMergerLogic(QObject *parent) : QObject(parent), workerThread(nullptr), worker(nullptr)
{
}

FileMergerLogic::~FileMergerLogic()
{
    if (workerThread) {
        if (workerThread->isRunning()) {
            workerThread->requestInterruption(); // Request interruption
            workerThread->quit(); // Request thread to quit
            if (!workerThread->wait(3000)) { // Wait for max 3 seconds
                 qWarning() << "Worker thread did not quit gracefully, terminating.";
                 workerThread->terminate();
                 workerThread->wait(); // Wait for termination
            }
        }
        delete workerThread; // QThread should be deleted after worker
        workerThread = nullptr;
    }
    // Worker object (worker) is moved to the thread and should be deleted
    // when the thread finishes via QObject::deleteLater.
    // If worker was not deleted for some reason, it would be a leak.
    // The connections worker->deleteLater() and thread->deleteLater() handle this.
     if(worker) { // Should have been deleted by deleteLater
        qWarning() << "Worker object was not deleted by thread finishing, deleting now.";
        delete worker; // Fallback, though typically deleteLater should handle it.
        worker = nullptr;
    }
}

void FileMergerLogic::startMergeProcess(const QStringList &files, const QString &outputDir)
{
    if (workerThread && workerThread->isRunning()) {
        emit statusUpdated(QCoreApplication::tr("合并操作已在进行中。 (Merge operation already in progress.)"));
        return;
    }

    // Clean up previous thread and worker if they exist and didn't self-delete
    // This is a safeguard; deleteLater connections should handle it.
    if (workerThread) {
        qDebug() << "Previous workerThread found, deleting it.";
        delete workerThread; // This will also trigger worker's deleteLater if connected
        workerThread = nullptr;
        worker = nullptr; // Worker should be null after thread deletion
    }


    workerThread = new QThread(this); // Parent thread to FileMergerLogic for safety if not deleted properly
    worker = new MergeWorker(files, outputDir);

    worker->moveToThread(workerThread);

    // Connections for thread lifecycle and worker signals
    connect(workerThread, &QThread::started, worker, &MergeWorker::process);
    connect(worker, &MergeWorker::finished, this, &FileMergerLogic::handleMergeWorkerFinished);
    connect(worker, &MergeWorker::progressUpdated, this, &FileMergerLogic::handleProgressUpdate);

    // When the worker is done, it emits finished(), which is connected to handleMergeWorkerFinished.
    // In handleMergeWorkerFinished, we can request the thread to quit.
    // And when the thread finishes, it should clean up the worker and itself.
    connect(worker, &MergeWorker::finished, worker, &QObject::deleteLater); // Worker cleans itself up
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater); // Thread cleans itself up


    // Also connect thread's finished signal to nullify our pointers
    // to avoid dangling pointers if the thread finishes unexpectedly.
    connect(workerThread, &QThread::finished, [this](){
        qDebug() << "Worker thread finished signal received. Worker pointer:" << worker << "Thread pointer:" << workerThread;
        // Objects are scheduled for deletion, so we nullify our tracking pointers.
        // Do not delete them here again if deleteLater is connected.
        this->worker = nullptr;
        this->workerThread = nullptr;
        qDebug() << "Worker and workerThread pointers nullified.";
    });


    workerThread->start();
    emit statusUpdated(QCoreApplication::tr("开始文件合并线程... (Starting file merge thread...)"));
}

void FileMergerLogic::handleMergeWorkerFinished(bool success, const QString &messageOrPath)
{
    emit mergeFinished(success, messageOrPath); // Forward signal to MainWindow

    // Thread and worker will be deleted due to deleteLater connections.
    // We request the thread to quit, which will eventually lead to its 'finished' signal.
    if (workerThread && workerThread->isRunning()) {
        workerThread->quit();
    }
    // Pointers are nullified in the QThread::finished lambda connection.
}

void FileMergerLogic::handleProgressUpdate(int percentage) {
    emit progressUpdated(percentage); // Forward to MainWindow
} 