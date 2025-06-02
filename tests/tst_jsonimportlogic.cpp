#include "tst_jsonimportlogic.h"

#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths> // For typical locations if needed, though tempDir is better

// This is the core logic extracted for testability (or simulated)
// Based on MainWindow::importFromJson
QStringList TestJsonImportLogic::parseAndValidatePaths(
    const QByteArray& jsonData,
    const QString& jsonVirtualPath, // Path of the JSON file itself, for relative path resolution
    QStringList& errorMessages)     // Output parameter for errors
{
    errorMessages.clear();
    QStringList validFilesPaths;

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        errorMessages.append(QString("JSON Parse Error: %1 at offset %2").arg(parseError.errorString(), QString::number(parseError.offset)));
        return validFilesPaths; // Empty list
    }

    if (!jsonDoc.isObject()) {
        errorMessages.append("Invalid JSON Format: Root is not an object.");
        return validFilesPaths;
    }

    QJsonObject rootObject = jsonDoc.object();
    const QString filesKey = "files_to_merge";

    if (!rootObject.contains(filesKey) || !rootObject[filesKey].isArray()) {
        errorMessages.append(QString("Invalid JSON Structure: Must contain '%1' key with an array.").arg(filesKey));
        return validFilesPaths;
    }

    QJsonArray fileArray = rootObject[filesKey].toArray();
    QDir jsonFileDir(QFileInfo(jsonVirtualPath).absolutePath());

    if (fileArray.isEmpty()){
        // This is not an error per se, just an empty list of files.
        // errorMessages.append("Empty files array."); // Or handle as success with no files.
    }

    for (const QJsonValue &val : fileArray) {
        if (!val.isString()) {
            errorMessages.append("Skipping non-string entry in file list.");
            continue;
        }

        QString pathFromFile = val.toString();
        QString absolutePath;

        QFileInfo fileInfoForPath(pathFromFile); // Use a different QFileInfo for the path string
        if (fileInfoForPath.isAbsolute()) {
            absolutePath = pathFromFile;
        } else {
            absolutePath = jsonFileDir.absoluteFilePath(pathFromFile);
        }
        absolutePath = QDir::cleanPath(absolutePath);


        QFileInfo fileCheck(absolutePath); // Use QFileInfo for file system checks
        if (!fileCheck.exists()) {
            errorMessages.append(QString("File not found: %1 (resolved from %2)").arg(absolutePath, pathFromFile));
            continue;
        }
        if (!fileCheck.isFile()){
             errorMessages.append(QString("Path is not a file: %1 (resolved from %2)").arg(absolutePath, pathFromFile));
            continue;
        }
        // Permission check is tricky for unit tests, skipping for now.

        validFilesPaths.append(absolutePath);
    }
    return validFilesPaths;
}


TestJsonImportLogic::TestJsonImportLogic() {}

TestJsonImportLogic::~TestJsonImportLogic() {}

void TestJsonImportLogic::initTestCase()
{
    QVERIFY(tempDir.isValid());
    // Create some dummy files and directories for tests
    testFile1Path = tempDir.filePath("testfile1.txt");
    QFile file1(testFile1Path);
    QVERIFY(file1.open(QIODevice::WriteOnly)); file1.write("content"); file1.close();

    testFile2Path = tempDir.filePath("another file.log"); // With space
    QFile file2(testFile2Path);
    QVERIFY(file2.open(QIODevice::WriteOnly)); file2.write("log data"); file2.close();

    testDirPath = tempDir.filePath("subfolder");
    QDir().mkpath(testDirPath); // Create a directory
    QVERIFY(QFileInfo(testDirPath).isDir());

    nestedFileInTestDir = QDir(testDirPath).filePath("nested.dat");
    QFile file3(nestedFileInTestDir);
    QVERIFY(file3.open(QIODevice::WriteOnly)); file3.write("nested"); file3.close();

}

void TestJsonImportLogic::cleanupTestCase()
{
    // tempDir will be automatically removed
}

void TestJsonImportLogic::testValidJsonAndPaths()
{
    QString jsonContent = QString(R"({ "files_to_merge": ["%1", "%2"] })")
                            .arg(testFile1Path)
                            .arg(testFile2Path);

    QStringList errors;
    QString dummyJsonPath = tempDir.filePath("test.json"); // The JSON file itself doesn't need to exist for this logic
    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPath, errors);

    QVERIFY2(errors.isEmpty(), errors.join("\n").toUtf8());
    QCOMPARE(validPaths.count(), 2);
    QVERIFY(validPaths.contains(QFileInfo(testFile1Path).absoluteFilePath()));
    QVERIFY(validPaths.contains(QFileInfo(testFile2Path).absoluteFilePath()));
}

void TestJsonImportLogic::testJsonWithRelativePaths()
{
    // JSON will be "in" tempDir.path()
    // files are "subfolder/nested.dat" and "testfile1.txt"
    QString jsonContent = QString(R"({ "files_to_merge": ["testfile1.txt", "subfolder/nested.dat", "./another file.log"] })");

    QStringList errors;
    // Simulate json file being in the root of tempDir
    QString dummyJsonPathInTempDir = tempDir.filePath("myconfig.json");

    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPathInTempDir, errors);

    QVERIFY2(errors.isEmpty(), errors.join("\n").toUtf8());
    QCOMPARE(validPaths.count(), 3);
    QVERIFY(validPaths.contains(QFileInfo(testFile1Path).absoluteFilePath()));
    QVERIFY(validPaths.contains(QFileInfo(nestedFileInTestDir).absoluteFilePath()));
    QVERIFY(validPaths.contains(QFileInfo(testFile2Path).absoluteFilePath()));
}


void TestJsonImportLogic::testJsonWithNonExistentFiles()
{
    QString nonExistent = tempDir.filePath("ghost.txt");
    QString jsonContent = QString(R"({ "files_to_merge": ["%1", "%2"] })")
                            .arg(testFile1Path)
                            .arg(nonExistent);

    QStringList errors;
    QString dummyJsonPath = tempDir.filePath("test.json");
    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPath, errors);

    QCOMPARE(errors.count(), 1);
    QVERIFY(errors.first().contains("File not found"));
    QVERIFY(errors.first().contains(QDir::cleanPath(nonExistent)));

    QCOMPARE(validPaths.count(), 1);
    QVERIFY(validPaths.contains(QFileInfo(testFile1Path).absoluteFilePath()));
}

void TestJsonImportLogic::testJsonWithInvalidEntries()
{
    // testDirPath is a directory, not a file.
    QString jsonContent = QString(R"({ "files_to_merge": [123, "%1", {"obj": "not a string"}, "%2"] })")
                            .arg(testFile1Path)
                            .arg(testDirPath); // Add directory path

    QStringList errors;
    QString dummyJsonPath = tempDir.filePath("test.json");
    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPath, errors);

    QCOMPARE(validPaths.count(), 1); // Only testFile1Path
    QVERIFY(validPaths.contains(QFileInfo(testFile1Path).absoluteFilePath()));

    // Expect 3 errors: non-string number, non-string object, path is not a file
    QCOMPARE(errors.count(), 3);
    QVERIFY(errors.filter("Skipping non-string entry").count() == 2);
    QVERIFY(errors.filter("Path is not a file").count() == 1);
    QVERIFY(errors.filter(QDir::cleanPath(testDirPath)).count() == 1); // Ensure the error message contains the cleaned path

}

void TestJsonImportLogic::testInvalidJsonStructure_MissingKey()
{
    QString jsonContent = R"({ "another_key": ["path/to/file.txt"] })";
    QStringList errors;
    QString dummyJsonPath = tempDir.filePath("test.json");
    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPath, errors);

    QVERIFY(!errors.isEmpty());
    QVERIFY(errors.first().contains("Invalid JSON Structure"));
    QVERIFY(errors.first().contains("files_to_merge"));
    QVERIFY(validPaths.isEmpty());
}

void TestJsonImportLogic::testInvalidJsonStructure_WrongType()
{
    QString jsonContent = R"({ "files_to_merge": "not-an-array" })";
     QStringList errors;
    QString dummyJsonPath = tempDir.filePath("test.json");
    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPath, errors);

    QVERIFY(!errors.isEmpty());
    QString expectedError = QString("Invalid JSON Structure: Must contain '%1' key with an array.").arg("files_to_merge");
    QCOMPARE(errors.first(), expectedError);
    QVERIFY(validPaths.isEmpty());
}

void TestJsonImportLogic::testMalformedJson()
{
    QString jsonContent = R"({ "files_to_merge": ["file1.txt", ])"; // Malformed, trailing comma in array
    QStringList errors;
    QString dummyJsonPath = tempDir.filePath("test.json");
    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPath, errors);

    QVERIFY(!errors.isEmpty());
    QVERIFY(errors.first().contains("JSON Parse Error"));
    QVERIFY(validPaths.isEmpty());
}

void TestJsonImportLogic::testEmptyFilesArray()
{
    QString jsonContent = R"({ "files_to_merge": [] })";
    QStringList errors;
    QString dummyJsonPath = tempDir.filePath("test.json");
    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPath, errors);

    QVERIFY2(errors.isEmpty(), errors.join("\n").toUtf8());
    QVERIFY(validPaths.isEmpty());
}

void TestJsonImportLogic::testJsonWithPathNormalization()
{
    // Path for dummy JSON file itself
    QString dummyJsonDir = tempDir.filePath("config_dir");
    QDir().mkpath(dummyJsonDir);
    QString dummyJsonPath = QDir(dummyJsonDir).filePath("myconfig.json");

    // testFile1Path is absolute. Create a file relative to dummyJsonDir
    // testFile1Path is tempDir.filePath("testfile1.txt")
    // dummyJsonPath is tempDir.filePath("config_dir/myconfig.json")
    // So, relative path from dummyJsonPath to testFile1Path is "../testfile1.txt"
    QString relativeFile = "../testfile1.txt";

    QString jsonContent = QString(R"({ "files_to_merge": ["%1"] })").arg(relativeFile);

    QStringList errors;
    QStringList validPaths = parseAndValidatePaths(jsonContent.toUtf8(), dummyJsonPath, errors);

    QVERIFY2(errors.isEmpty(), errors.join("\n").toUtf8());
    QCOMPARE(validPaths.count(), 1);
    QCOMPARE(validPaths.first(), QFileInfo(testFile1Path).absoluteFilePath());
}


// QTEST_APPLESS_MAIN(TestJsonImportLogic) // This would make it a standalone test executable
                                        // If it's part of a larger suite, this is not needed here.
                                        // The main() in tst_customfilemodel.cpp will run it.
// moc_tst_jsonimportlogic.cpp will be generated from tst_jsonimportlogic.h and compiled separately.
