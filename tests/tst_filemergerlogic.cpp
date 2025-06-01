// tests/tst_filemergerlogic.cpp
#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QStringList>
#include <QSignalSpy>
#include <QThread> // Required for FileMergerLogic tests
#include <QDateTime>
#include <QRegularExpression>

// Include the classes to be tested
// Adjust paths as necessary
#include "../src/filemergerlogic.h" // For FileMergerLogic and MergeWorker

class TestFileMergerLogic : public QObject
{
    Q_OBJECT

public:
    TestFileMergerLogic();
    ~TestFileMergerLogic();

private slots:
    // Lifecycle methods
    void initTestCase();    // Called before the first test function
    void cleanupTestCase(); // Called after the last test function
    void init();            // Called before each test function
    void cleanup();         // Called after each test function

    // Test cases for MergeWorker
    void testMergeWorker_Success();
    void testMergeWorker_NoFiles();
    void testMergeWorker_FileDoesNotExist();
    void testMergeWorker_FileUnreadable();
    void testMergeWorker_ProgressSignal();
    void testMergeWorker_Cancellation();
    // void testMergeWorker_OutputNamingAndContent(); // Covered by testMergeWorker_Success

    // Test cases for FileMergerLogic
    void testFileMergerLogic_StartProcess();
    void testFileMergerLogic_Signals();
    void testFileMergerLogic_MergeInProgress();
    void testFileMergerLogic_ThreadCleanup();


private:
    QTemporaryDir *tempDir; // For creating temporary files/folders for tests
    // Helper to create dummy files
    QString createFile(const QString& dirPath, const QString& fileName, const QString& content);
};

TestFileMergerLogic::TestFileMergerLogic() : tempDir(nullptr)
{
}

TestFileMergerLogic::~TestFileMergerLogic()
{
    // tempDir is cleaned up in cleanupTestCase or by its own destructor if allocated in init
}

void TestFileMergerLogic::initTestCase()
{
    qDebug() << "Starting test suite for FileMergerLogic.";
    // Global setup for all tests in this class
}

void TestFileMergerLogic::cleanupTestCase()
{
    qDebug() << "Finished test suite for FileMergerLogic.";
    // Global cleanup
}

void TestFileMergerLogic::init()
{
    // Setup before each test function
    tempDir = new QTemporaryDir();
    QVERIFY(tempDir->isValid());
}

void TestFileMergerLogic::cleanup()
{
    // Cleanup after each test function
    if (tempDir) {
        delete tempDir;
        tempDir = nullptr;
    }
}

QString TestFileMergerLogic::createFile(const QString& dirPath, const QString& fileName, const QString& content)
{
    QFile file(QDir(dirPath).filePath(fileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing in createFile:" << file.fileName() << file.errorString();
        return QString(); // Return empty string on failure
    }
    QTextStream out(&file);
    out << content;
    file.close();
    return file.fileName();
}

// --- MergeWorker Test Implementations ---
void TestFileMergerLogic::testMergeWorker_Success()
{
    QString file1Content = "This is file 1.";
    QString file2Content = "This is file 2.\nWith a new line.";
    QString file3Content = "And file 3 here.";

    QString file1 = createFile(tempDir->path(), "file1.txt", file1Content);
    QString file2 = createFile(tempDir->path(), "file2.txt", file2Content);
    QString file3 = createFile(tempDir->path(), "file3.txt", file3Content);

    QStringList filesToMerge = {file1, file2, file3};
    QString outputDir = tempDir->path();

    MergeWorker *worker = new MergeWorker(filesToMerge, outputDir);
    QSignalSpy spy(worker, &MergeWorker::finished);

    worker->process(); // This is synchronous in the current design

    // Attempt to manually process events for a short duration if spy.wait is problematic
    if (spy.count() == 0) { // Only if signal not already received (e.g. via DirectConnection)
        for (int i = 0; i < 10; ++i) { // Loop for a short time
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50); // Process events for 50ms
            if (spy.count() > 0) break; // Stop if signal received
            QTest::qWait(10); // Small delay between event processing attempts
        }
    }
    // Fallback to spy.wait() if still not received, or verify immediately if count > 0
    if (spy.count() == 0) {
      QVERIFY(spy.wait(1000)); // Shorter wait now if above loop was tried
    }
    QCOMPARE(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true); // Success

    QString outputFilePath = arguments.at(1).toString();
    QVERIFY(QFile::exists(outputFilePath));

    // Check output file naming
    QFileInfo fileInfo(outputFilePath);
    QString baseName = fileInfo.fileName();
    QVERIFY(baseName.startsWith("collated_files_"));
    QVERIFY(baseName.endsWith(".txt"));
    QRegularExpression tsRegex("_\\d{4}-\\d{2}-\\d{2}_\\d{2}-\\d{2}-\\d{2}\\.txt$");
    QVERIFY(baseName.contains(tsRegex));


    QFile outputFile(outputFilePath);
    QVERIFY(outputFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString mergedContent = QTextStream(&outputFile).readAll();
    outputFile.close();

    QString expectedContent;
    QTextStream expectedStream(&expectedContent);
    expectedStream << "========== [" << QFileInfo(file1).fileName() << "] ==========\n\n" << file1Content << "\n\n";
    expectedStream << "========== [" << QFileInfo(file2).fileName() << "] ==========\n\n" << file2Content << "\n\n";
    expectedStream << "========== [" << QFileInfo(file3).fileName() << "] ==========\n\n" << file3Content << "\n\n";
    expectedStream.flush();

    QCOMPARE(mergedContent.trimmed(), expectedContent.trimmed());

    delete worker;
}

void TestFileMergerLogic::testMergeWorker_NoFiles()
{
    QStringList filesToMerge;
    QString outputDir = tempDir->path();

    MergeWorker *worker = new MergeWorker(filesToMerge, outputDir);
    QSignalSpy spy(worker, &MergeWorker::finished);

    worker->process();
    if (spy.count() == 0) {
        for (int i = 0; i < 5; ++i) { QCoreApplication::processEvents(QEventLoop::AllEvents, 50); if (spy.count() > 0) break; QTest::qWait(10); }
    }
    if (spy.count() == 0) { QVERIFY(spy.wait(500)); }
    QCOMPARE(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);
    QVERIFY(arguments.at(1).toString().contains(QCoreApplication::tr("没有选择文件进行合并。")));

    delete worker;
}

void TestFileMergerLogic::testMergeWorker_FileDoesNotExist()
{
    QString file1Content = "Valid file content.";
    QString validFile = createFile(tempDir->path(), "valid.txt", file1Content);
    QString nonExistentFile = QDir(tempDir->path()).filePath("non_existent_file.txt");

    QStringList filesToMerge = {validFile, nonExistentFile};
    QString outputDir = tempDir->path();

    MergeWorker *worker = new MergeWorker(filesToMerge, outputDir);
    QSignalSpy spy(worker, &MergeWorker::finished);

    worker->process();
    if (spy.count() == 0) {
        for (int i = 0; i < 10; ++i) { QCoreApplication::processEvents(QEventLoop::AllEvents, 50); if (spy.count() > 0) break; QTest::qWait(10); }
    }
    if (spy.count() == 0) { QVERIFY(spy.wait(1000)); }
    QCOMPARE(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);

    QString outputFilePath = arguments.at(1).toString();
    QVERIFY(QFile::exists(outputFilePath));

    QFile outputFile(outputFilePath);
    QVERIFY(outputFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString mergedContent = QTextStream(&outputFile).readAll();
    outputFile.close();

    QString expectedContent;
    QTextStream expectedStream(&expectedContent);
    expectedStream << "========== [" << QFileInfo(validFile).fileName() << "] ==========\n\n" << file1Content << "\n\n";
    expectedStream.flush();

    QCOMPARE(mergedContent.trimmed(), expectedContent.trimmed());

    delete worker;
}

void TestFileMergerLogic::testMergeWorker_ProgressSignal()
{
    QString file1 = createFile(tempDir->path(), "f1.txt", "content1");
    QString file2 = createFile(tempDir->path(), "f2.txt", "content2");
    QString file3 = createFile(tempDir->path(), "f3.txt", "content3");
    QStringList filesToMerge = {file1, file2, file3};

    MergeWorker *worker = new MergeWorker(filesToMerge, tempDir->path());
    QSignalSpy progressSpy(worker, &MergeWorker::progressUpdated);
    QSignalSpy finishedSpy(worker, &MergeWorker::finished);

    worker->process(); // Synchronous call
    if (finishedSpy.count() == 0) {
        for (int i = 0; i < 5; ++i) { QCoreApplication::processEvents(QEventLoop::AllEvents, 50); if (finishedSpy.count() > 0) break; QTest::qWait(10); }
    }
    if (finishedSpy.count() == 0) { QVERIFY(finishedSpy.wait(500)); }
    QVERIFY(finishedSpy.count() == 1);

    QVERIFY(progressSpy.count() >= 2);

    bool firstIsZero = false;
    if (!progressSpy.isEmpty()) {
        firstIsZero = (progressSpy.first().at(0).toInt() == 0);
    }
    QVERIFY(firstIsZero);

    bool lastIsHundred = false;
    if (!progressSpy.isEmpty()) {
        lastIsHundred = (progressSpy.last().at(0).toInt() == 100);
    }
    QVERIFY(lastIsHundred);

    if (progressSpy.count() > 2) {
        bool intermediateFound = false;
        for (int i = 1; i < progressSpy.count() - 1; ++i) {
            int val = progressSpy.at(i).at(0).toInt();
            if (val > 0 && val < 100) {
                intermediateFound = true;
                break;
            }
        }
        QVERIFY(intermediateFound);
    }
    delete worker;
}

void TestFileMergerLogic::testMergeWorker_Cancellation()
{
    QStringList filesToMerge;
    for (int i = 0; i < 10; ++i) {
        filesToMerge << createFile(tempDir->path(), QString("cancel_file%1.txt").arg(i), QString("Content of file %1").arg(i));
    }

    MergeWorker *worker = new MergeWorker(filesToMerge, tempDir->path());
    QThread *thread = new QThread();
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &MergeWorker::process);
    connect(worker, &MergeWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    QSignalSpy finishedSpy(worker, &MergeWorker::finished);

    thread->start();
    QTest::qWait(100);

    QVERIFY(thread->isRunning());
    thread->requestInterruption();

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    QList<QVariant> arguments = finishedSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);
    QVERIFY(arguments.at(1).toString().contains(QCoreApplication::tr("合并操作已取消。")));

    if (thread->isRunning()) {
        thread->quit();
        thread->wait(1000);
    }
    delete worker;
}


void TestFileMergerLogic::testMergeWorker_FileUnreadable()
{
    QString unreadableFileName = "unreadable.txt";
    QString unreadableFilePath = QDir(tempDir->path()).filePath(unreadableFileName);
    createFile(tempDir->path(), unreadableFileName, "This content should not be read.");

    QString readableFile = createFile(tempDir->path(), "readable.txt", "Readable content.");
    QStringList filesToMerge = {unreadableFilePath, readableFile};

    QFile::Permissions originalPermissions = QFile::permissions(unreadableFilePath);
    bool permissionsSet = QFile::setPermissions(unreadableFilePath, QFileDevice::Permissions());

    if (!permissionsSet && QFile::permissions(unreadableFilePath) != QFileDevice::Permissions() ) {
         QWARN("Could not make the file unreadable. Skipping core assertions of testMergeWorker_FileUnreadable.");
         QFile::setPermissions(unreadableFilePath, originalPermissions);
         QSKIP("File permissions could not be modified for testing unreadability.", SkipSingle);
    }

    MergeWorker *worker = new MergeWorker(filesToMerge, tempDir->path());
    QSignalSpy spy(worker, &MergeWorker::finished);

    worker->process();
    if (spy.count() == 0) {
        for (int i = 0; i < 10; ++i) { QCoreApplication::processEvents(QEventLoop::AllEvents, 50); if (spy.count() > 0) break; QTest::qWait(10); }
    }
    if (spy.count() == 0) { QVERIFY(spy.wait(1000)); }
    QCOMPARE(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();

    bool success = arguments.at(0).toBool();
    QString message = arguments.at(1).toString();

    if (QFile::permissions(unreadableFilePath) == QFileDevice::Permissions()) {
        QVERIFY(success == false);
        // Use the exact file path string used when emitting the signal for prefix comparison
        QString expectedPrefix = QCoreApplication::tr("无法读取文件: (Could not read file:) ") + unreadableFilePath;
        QVERIFY(message.startsWith(expectedPrefix));
    } else {
        qWarning() << "File permissions were not fully restrictive as expected during testMergeWorker_FileUnreadable. Current permissions:" << QFile::permissions(unreadableFilePath);
        if (permissionsSet) {
             QVERIFY(success == false);
             QString expectedPrefix = QCoreApplication::tr("无法读取文件: (Could not read file:) ") + unreadableFilePath;
             QVERIFY(message.startsWith(expectedPrefix));
        } else {
             QWARN("Test logic anomaly in testMergeWorker_FileUnreadable: permissions issue not caught by QSKIP.");
        }
    }

    QFile::setPermissions(unreadableFilePath, originalPermissions);
    delete worker;
}

// --- FileMergerLogic Test Implementations ---
void TestFileMergerLogic::testFileMergerLogic_StartProcess()
{
    FileMergerLogic logic;

    QString file1 = createFile(tempDir->path(), "logic_f1.txt", "Logic file 1 content.");
    QString file2 = createFile(tempDir->path(), "logic_f2.txt", "Logic file 2 content.");
    QStringList filesToMerge = {file1, file2};
    QString outputDir = tempDir->path();

    QSignalSpy mergeFinishedSpy(&logic, &FileMergerLogic::mergeFinished);

    logic.startMergeProcess(filesToMerge, outputDir);

    QVERIFY(mergeFinishedSpy.wait(5000));
    QCOMPARE(mergeFinishedSpy.count(), 1);

    QList<QVariant> arguments = mergeFinishedSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);

    QString outputFilePath = arguments.at(1).toString();
    QVERIFY(QFile::exists(outputFilePath));

    // Verify content (restored)
    QFile outputFile(outputFilePath);
    QVERIFY(outputFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString mergedContent = QTextStream(&outputFile).readAll();
    outputFile.close();
    qDebug() << "testFileMergerLogic_StartProcess - OutputFilePath:" << outputFilePath;
    qDebug() << "testFileMergerLogic_StartProcess - MergedContent length:" << mergedContent.length();
    QVERIFY(mergedContent.contains("Logic file 1 content."));
    QVERIFY(mergedContent.contains("Logic file 2 content."));
    QVERIFY(mergedContent.contains("========== [logic_f1.txt] =========="));
}

void TestFileMergerLogic::testFileMergerLogic_Signals()
{
    FileMergerLogic logic;

    QString file1 = createFile(tempDir->path(), "sig_f1.txt", "Signal test file 1.");
    QString file2 = createFile(tempDir->path(), "sig_f2.txt", "Signal test file 2.");
    QStringList filesToMerge = {file1, file2};

    QSignalSpy statusSpy(&logic, &FileMergerLogic::statusUpdated);
    QSignalSpy progressSpy(&logic, &FileMergerLogic::progressUpdated);
    QSignalSpy finishedSpy(&logic, &FileMergerLogic::mergeFinished);

    logic.startMergeProcess(filesToMerge, tempDir->path());

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    QVERIFY(!statusSpy.isEmpty());
    bool startingMessageFound = false;
    for(const auto& item : statusSpy) {
        if (item.at(0).toString().contains(QCoreApplication::tr("开始文件合并线程..."))) { // Adjusted to match new log in FileMergerLogic
            startingMessageFound = true;
            break;
        }
    }
    QVERIFY(startingMessageFound);

    QVERIFY(progressSpy.count() >= 2);
    bool firstIsZero = !progressSpy.isEmpty() && progressSpy.first().at(0).toInt() == 0;
    QVERIFY(firstIsZero);
    bool lastIsHundred = !progressSpy.isEmpty() && progressSpy.last().at(0).toInt() == 100;
    QVERIFY(lastIsHundred);

    QList<QVariant> arguments = finishedSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    QVERIFY(QFile::exists(arguments.at(1).toString()));
}

void TestFileMergerLogic::testFileMergerLogic_MergeInProgress()
{
    FileMergerLogic logic;

    QStringList longFileList;
    for (int i = 0; i < 5; ++i) {
        longFileList << createFile(tempDir->path(), QString("long_f%1.txt").arg(i), QString("Long list file %1").arg(i));
    }
    QStringList shortFileList = {createFile(tempDir->path(), "short_f.txt", "Short list file.")};

    QSignalSpy statusSpy(&logic, &FileMergerLogic::statusUpdated);
    QSignalSpy finishedSpy(&logic, &FileMergerLogic::mergeFinished);

    logic.startMergeProcess(longFileList, tempDir->path());
    QTest::qWait(50);
    logic.startMergeProcess(shortFileList, tempDir->path() + "/out_short");

    QVERIFY(finishedSpy.wait(10000));
    QCOMPARE(finishedSpy.count(), 1);

    bool inProgressMessageFound = false;
    for (const auto& item : statusSpy) {
        if (item.at(0).toString().contains(QCoreApplication::tr("合并操作已在进行中。"))) {
            inProgressMessageFound = true;
            break;
        }
    }
    QVERIFY(inProgressMessageFound);

    QList<QVariant> arguments = finishedSpy.first();
    QVERIFY(arguments.at(0).toBool() == true);
    QString firstOutputPath = arguments.at(1).toString();
    QVERIFY(QFile::exists(firstOutputPath));

    QVERIFY(!QFile::exists(tempDir->path() + "/out_short/" + QFileInfo(shortFileList.first()).fileName()));
}

void TestFileMergerLogic::testFileMergerLogic_ThreadCleanup()
{
    FileMergerLogic logic;

    QString file1 = createFile(tempDir->path(), "cleanup_f1.txt", "Cleanup test 1");
    QStringList files1 = {file1};
    QString outputDir1 = QDir(tempDir->path()).filePath("out1");
    QDir().mkpath(outputDir1);

    QString file2 = createFile(tempDir->path(), "cleanup_f2.txt", "Cleanup test 2");
    QStringList files2 = {file2};
    QString outputDir2 = QDir(tempDir->path()).filePath("out2");
    QDir().mkpath(outputDir2);

    QSignalSpy finishedSpy(&logic, &FileMergerLogic::mergeFinished);

    logic.startMergeProcess(files1, outputDir1);
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);
    QList<QVariant> args1 = finishedSpy.takeFirst();
    QVERIFY(args1.at(0).toBool() == true);
    QVERIFY(QFile::exists(args1.at(1).toString()));

    logic.startMergeProcess(files2, outputDir2);
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);
    QList<QVariant> args2 = finishedSpy.takeFirst();
    QVERIFY(args2.at(0).toBool() == true);
    QString outputPath2 = args2.at(1).toString();
    QVERIFY(QFile::exists(outputPath2));
    QVERIFY(outputPath2.contains("out2"));
}

#include "tst_filemergerlogic.moc"
