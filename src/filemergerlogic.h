// filemergerlogic.h

#ifndef FILEMERGERLOGIC_H
#define FILEMERGERLOGIC_H

#include <QObject>
#include <QStringList>
#include <QPointer>  // Added for QPointer

class QThread; // Forward declaration

// Worker class that will run in a separate thread
class MergeWorker : public QObject {
    Q_OBJECT
public:
    MergeWorker(const QStringList &files, const QString &outputPath);
    ~MergeWorker();

public slots:
    void process(); // This is the slot that will be called when the thread starts

signals:
    void finished(bool success, const QString &messageOrPath);
    void progressUpdated(int percentage);

private:
    QStringList filesToMerge;
    QString outputPathBase; // e.g., Desktop path
};


class FileMergerLogic : public QObject
{
    Q_OBJECT
public:
    explicit FileMergerLogic(QObject *parent = nullptr);
    ~FileMergerLogic();

    void startMergeProcess(const QStringList &files, const QString &outputDir);

signals:
    void statusUpdated(const QString &message);
    void mergeFinished(bool success, const QString &messageOrPath);
    void progressUpdated(int percentage);

private slots:
    void handleMergeWorkerFinished(bool success, const QString &messageOrPath);
    void handleProgressUpdate(int percentage);

private:
    QThread *workerThread;
    QPointer<MergeWorker> worker; // Keep track of the worker, now a QPointer
};

#endif // FILEMERGERLOGIC_H 