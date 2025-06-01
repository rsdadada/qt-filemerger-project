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
#include <QPointer> // For QPointer

// --- MergeWorker Implementation ---
MergeWorker::MergeWorker(const QStringList &files, const QString &outputPath)
    : filesToMerge(files), outputPathBase(outputPath) {}

MergeWorker::~MergeWorker() {
    qDebug() << "MergeWorker destroyed";
}

void MergeWorker::process() {
    qDebug() << "MergeWorker::process() started. Thread:" << QThread::currentThread();
    if (filesToMerge.isEmpty()) {
        qDebug() << "MergeWorker::process() - No files to merge.";
        emit finished(false, QCoreApplication::tr("没有选择文件进行合并。 (No files selected for merging.)"));
        emit progressUpdated(100); // Or 0, depending on convention for "nothing to do"
        qDebug() << "MergeWorker::process() finished - no files.";
        return;
    }

    qDebug() << "MergeWorker::process() - Processing" << filesToMerge.count() << "files.";
    QString mergedContent;
    int fileCount = filesToMerge.count();
    int processedCount = 0;
    emit progressUpdated(0);
    qDebug() << "MergeWorker::process() - Initial progress 0 emitted.";

    for (int i = 0; i < filesToMerge.count(); ++i) {
        const QString &filePath = filesToMerge.at(i);
        qDebug() << "MergeWorker::process() - Processing file:" << filePath;
        if (QThread::currentThread()->isInterruptionRequested()) {
             qDebug() << "MergeWorker::process() - Interruption requested. Emitting finished(false).";
             emit finished(false, QCoreApplication::tr("合并操作已取消。(Merge operation cancelled.)"));
             emit progressUpdated(processedCount * 100 / fileCount); // Current progress before abort
             return;
        }

        QFile file(filePath);
        if (!file.exists()) {
            qWarning() << "File does not exist, skipping:" << filePath;
            processedCount++;
            emit progressUpdated((processedCount * 100) / fileCount);
            qDebug() << "MergeWorker::process() - File skipped, progress updated.";
            continue;
        }
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "MergeWorker::process() - Cannot open file:" << filePath << ". Error:" << file.errorString();
            emit finished(false, QCoreApplication::tr("无法读取文件: (Could not read file:) ") + filePath + "\n" + file.errorString());
            emit progressUpdated((processedCount * 100) / fileCount);
            qDebug() << "MergeWorker::process() - File open error, emitted finished(false).";
            return;
        }
        qDebug() << "MergeWorker::process() - File opened:" << filePath;

        QFileInfo fileInfo(filePath);
        mergedContent.append(QString("\n\n========== [%1] ==========\n\n").arg(fileInfo.fileName())); // Note: No space after ========

        QTextStream in(&file);
        mergedContent.append(in.readAll());
        file.close();
        qDebug() << "MergeWorker::process() - File read and closed:" << filePath;

        processedCount++;
        emit progressUpdated((processedCount * 100) / fileCount);
        qDebug() << "MergeWorker::process() - Progress updated to" << (processedCount * 100) / fileCount;

        // Artificially slow down for cancellation test predictability for the first few files
        if (i < 5) { // Affects first 5 files
            QThread::msleep(50); // Sleep for 50ms
        }
    }

    if (QThread::currentThread()->isInterruptionRequested()) {
         qDebug() << "MergeWorker::process() - Interruption requested before saving. Emitting finished(false).";
         emit finished(false, QCoreApplication::tr("合并操作已取消。(Merge operation cancelled before saving.)"));
         return;
    }

    qDebug() << "MergeWorker::process() - All files processed. Preparing to write output.";
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString outputFilename = QString("collated_files_%1.txt").arg(timestamp);
    QString outputFilePath = QDir(outputPathBase).filePath(outputFilename);
    qDebug() << "MergeWorker::process() - Output path:" << outputFilePath;

    QFile outputFile(outputFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "MergeWorker::process() - Cannot create output file:" << outputFilePath << ". Error:" << outputFile.errorString();
        emit finished(false, QCoreApplication::tr("无法创建输出文件: (Could not create output file:) ") + outputFilePath + "\n" + outputFile.errorString());
        emit progressUpdated(100); // Indicate process attempted completion
        qDebug() << "MergeWorker::process() - Output file creation error, emitted finished(false).";
        return;
    }

    QTextStream out(&outputFile);
    out << mergedContent;
    outputFile.close();
    qDebug() << "MergeWorker::process() - Output file written and closed.";

    emit progressUpdated(100);
    qDebug() << "MergeWorker::process() - Emitting finished(true).";
    emit finished(true, outputFilePath);
    qDebug() << "MergeWorker::process() finished successfully.";
}


// --- FileMergerLogic Implementation ---
FileMergerLogic::FileMergerLogic(QObject *parent) : QObject(parent), workerThread(nullptr), worker(nullptr)
{
}

FileMergerLogic::~FileMergerLogic() {
    qDebug() << "FileMergerLogic Destructor for" << this << ". Current workerThread:" << workerThread;
    if (workerThread) { // Check if a thread object was ever assigned
        if (workerThread->isRunning()) {
            qDebug() << "Destructor: Thread is running. Requesting interruption, quit.";
            workerThread->requestInterruption();
            workerThread->quit();
            qDebug() << "Destructor: Waiting for thread to finish...";
            if (!workerThread->wait(1500)) { // Increased wait slightly
                 qWarning() << "Destructor: Worker thread " << workerThread << " did not quit gracefully for " << this;
            } else {
                 qDebug() << "Destructor: Worker thread " << workerThread << " quit gracefully for " << this;
            }
        } else {
            qDebug() << "Destructor: Thread was not running.";
        }
        // Process events one last time after wait or if thread wasn't running,
        // to help ensure deleteLater slots and the lambda connected to QThread::finished are processed.
        qDebug() << "Destructor: Processing events to clear pending signals/deleteLater for thread " << workerThread;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 200);
    }
    // Pointers should be null if lambda ran successfully and object wasn't prematurely deleted.
    // Nullifying them here is a final step regardless.
    worker = nullptr;
    workerThread = nullptr;
    qDebug() << "FileMergerLogic Destructor finished for" << this;
}

void FileMergerLogic::startMergeProcess(const QStringList &files, const QString &outputDir)
{
    if (workerThread && workerThread->isRunning()) {
        emit statusUpdated(QCoreApplication::tr("合并操作已在进行中。 (Merge operation already in progress.)"));
        qDebug() << "StartMergeProcess: Merge operation already in progress. Bailing out.";
        return;
    }

    if (worker || workerThread) {
         qWarning() << "StartMergeProcess: Previous worker (" << worker << ") or workerThread (" << workerThread
                    << ") pointers are not null. This implies they are pending deleteLater or lambda was not connected/run."
                    << " Overwriting pointers for new merge operation.";
         // Old objects should eventually self-delete due to their deleteLater connections.
    }

    workerThread = new QThread();
    worker = new MergeWorker(files, outputDir);

    worker->moveToThread(workerThread);

    // Connections for thread lifecycle and worker signals
    connect(workerThread, &QThread::started, worker, &MergeWorker::process);
    connect(worker, &MergeWorker::finished, this, &FileMergerLogic::handleMergeWorkerFinished);
    connect(worker, &MergeWorker::progressUpdated, this, &FileMergerLogic::handleProgressUpdate);

    // When the worker is done, it emits finished(), which is connected to handleMergeWorkerFinished.
    // In handleMergeWorkerFinished, we can request the thread to quit.
    // And when the thread finishes, it should clean up the worker and itself.
    connect(worker, &MergeWorker::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    // Restore the QPointer-based lambda
    QPointer<FileMergerLogic> safeThis(this);
    connect(workerThread, &QThread::finished, [safeThis]() {
        qDebug() << "LAMBDA: QThread::finished received. safeThis is" << (safeThis ? "valid" : "null");
        if (safeThis) {
            qDebug() << "LAMBDA: safeThis is valid. Nullifying FileMergerLogic's worker (" << safeThis->worker << ") and workerThread (" << safeThis->workerThread << ") pointers.";
            safeThis->worker = nullptr;
            safeThis->workerThread = nullptr;
            qDebug() << "LAMBDA: Pointers nullified via safeThis.";
        } else {
            qDebug() << "LAMBDA: safeThis is null (FileMergerLogic instance already destroyed). Cannot nullify pointers.";
        }
        qDebug() << "LAMBDA: EXIT";
    });

    workerThread->start();
    emit statusUpdated(QCoreApplication::tr("开始文件合并线程... (Starting file merge thread...)"));
    qDebug() << "StartMergeProcess: Worker thread started. Worker:" << worker << "Thread:" << workerThread;
}

void FileMergerLogic::handleMergeWorkerFinished(bool success, const QString &messageOrPath)
{
    qDebug() << "FileMergerLogic::handleMergeWorkerFinished - Received from worker. Success:" << success << "Path:" << messageOrPath;

    MergeWorker* senderWorker = qobject_cast<MergeWorker*>(sender());

    // It's crucial to ensure this slot is acting on the correct worker/thread pair,
    // especially if a new merge operation could have been started, replacing this->worker and this->workerThread.
    if (!senderWorker || senderWorker != this->worker || !this->workerThread) {
        qWarning() << "FileMergerLogic::handleMergeWorkerFinished: Signal from an old/unexpected worker or thread is already cleaned up. Ignoring. Current this->worker:" << this->worker << "Sender:" << senderWorker;
        return;
    }

    QThread* threadToQuit = this->workerThread; // Capture the thread associated with this worker

    emit mergeFinished(success, messageOrPath);
    qDebug() << "FileMergerLogic::handleMergeWorkerFinished - Emitted mergeFinished. Requesting quit for thread:" << threadToQuit;

    if (threadToQuit && threadToQuit->isRunning()) { // Check if captured thread pointer is valid and running
        threadToQuit->quit();
    } else if (threadToQuit) {
        qDebug() << "FileMergerLogic::handleMergeWorkerFinished - Captured thread" << threadToQuit << "is not running. No need to quit.";
    }
    // When the lambda (that was temporarily removed) is restored, it would nullify this->worker and this->workerThread.
    // Without the lambda, these pointers will be nullified by the destructor or when startMergeProcess is called again.
}

void FileMergerLogic::handleProgressUpdate(int percentage) {
    emit progressUpdated(percentage); // Forward to MainWindow
} 