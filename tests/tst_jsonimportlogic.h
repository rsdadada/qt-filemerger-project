#ifndef TST_JSONIMPORTLOGIC_H
#define TST_JSONIMPORTLOGIC_H

#include <QObject>
#include <QStringList>
#include <QTemporaryDir> // For managing test files and paths

// Forward declare if needed, or include actual headers
class QJsonObject; // From <QJsonObject>
class QJsonArray;  // From <QJsonArray>


// Define a helper struct or class to hold results if the logic is complex
// For now, we can have the test methods return bool or use QCOMPARE directly
// with QStringList for paths and errors.

// Helper function (or class method) to encapsulate the logic being tested.
// This function would mirror the core part of MainWindow::importFromJson
// QStringList processJsonImportLogic(
//     const QByteArray& jsonData,
//     const QString& jsonVirtualPath, // Path of the JSON file itself, for relative path resolution
//     QStringList& errorMessages // Output parameter for errors
// );
// For now, we will implement the logic directly in test methods to avoid changing main code yet.


class TestJsonImportLogic : public QObject
{
    Q_OBJECT

public:
    TestJsonImportLogic();
    ~TestJsonImportLogic();

private Q_SLOTS:
    void initTestCase();    // Called before the first test function is executed
    void cleanupTestCase(); // Called after the last test function was executed

    void testValidJsonAndPaths();
    void testJsonWithRelativePaths();
    void testJsonWithNonExistentFiles();
    void testJsonWithInvalidEntries(); // e.g., non-string in files array
    void testInvalidJsonStructure_MissingKey();
    void testInvalidJsonStructure_WrongType();
    void testMalformedJson();
    void testEmptyFilesArray();
    void testJsonWithPathNormalization();


private:
    // Helper to simulate the core logic of MainWindow::importFromJson
    // It takes JSON data as a string, the path of the JSON file (for relative paths),
    // and outputs valid file paths and error messages.
    QStringList parseAndValidatePaths(const QByteArray& jsonData, const QString& jsonFilePath, QStringList& errorMessages);

    QTemporaryDir tempDir; // To create actual files for testing path validation
    QString testFile1Path;
    QString testFile2Path;
    QString nestedFileInTestDir;
    QString testDirPath;

};

#endif // TST_JSONIMPORTLOGIC_H
