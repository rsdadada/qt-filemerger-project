// tst_customfilemodel.cpp
#include <QtTest>
#include <QCoreApplication> // For QCoreApplication::tr (if used by CustomFileModel indirectly for default header data)
#include <QTemporaryDir>    // For managing temporary test files/folders
#include <QDir>             // For file operations within temporary directory
#include <QFile>            // For creating dummy files
#include <QSignalSpy>       // For testing signal emissions
#include <QStandardPaths>   // For robust temporary path handling if needed

// Include the class to be tested
// Adjust the path as necessary if your test file is in a different directory
#include "customfilemodel.h"
// You might also need to include treeitem.h if it's not fully opaque
// #include "treeitem.h"

// A common practice is to create a temporary directory for file system tests.
// For simplicity, this example assumes a valid path can be provided.
// In a real scenario, you'd use QTemporaryDir.

class TestCustomFileModel : public QObject
{
    Q_OBJECT

public:
    TestCustomFileModel();
    ~TestCustomFileModel();

private slots:
    void initTestCase();    // Called before the first test function
    void cleanupTestCase(); // Called after the last test function
    void init();            // Called before each test function
    void cleanup();         // Called after each test function

    // Initialization and Basic Structure
    void testInitialState_EmptyDir();
    void testModelConstruction_ValidPopulatedPath();
    void testModelConstruction_InvalidPath(); // Assumes graceful handling
    void testColumnCount();
    void testHeaderData_DisplayRole();

    // Item Navigation and Data Retrieval
    void testRowCount_EmptyDir();
    void testRowCount_PopulatedDir();
    void testHasFiles_EmptyDir();
    void testHasFiles_PopulatedDir();
    void testIndexAndParent_RootItems();
    void testIndexAndParent_NestedItems();
    void testData_DisplayRole();
    void testData_CheckStateRole_Initial();
    void testFlags_IsCheckable();

    // Check State Management
    void testSetData_CheckFile_EmitsSignal();
    void testSetData_CheckFolder_Propagation_EmitsSignal();
    void testSetData_UncheckFile_ParentUpdate_EmitsSignal();
    void testToggleCheckState_File();
    void testToggleCheckState_Folder_Propagation();
    void testSetAllCheckStates_Checked();
    void testSetAllCheckStates_Unchecked();

    // Path Retrieval
    void testGetCheckedFilesPaths_NoneChecked();
    void testGetCheckedFilesPaths_OnlyFilesChecked();
    void testGetCheckedFilesPaths_FolderChecked();
    void testGetCheckedFilesPaths_MixedContent();

    // Extension-based Selection
    void testSelectFilesByExtension_SpecificFolder_NoRecursion();
    void testSelectFilesByExtensionRecursive_FromRoot();
    void testSelectFilesByExtensionRecursive_FromSubfolder();


private:
    CustomFileModel *model;
    QTemporaryDir *tempDir; // <--- MODIFIED: Pointer
    QString originalModelRootPath;

    // Helper to create a common populated directory structure
    void createPopulatedTestDirectory(const QString& basePath);
    void createExtensionTestDirectory(const QString& basePath);

    // Helper to find item by name
    QModelIndex findItem(const QString& name, const QModelIndex& parent = QModelIndex()) const;
};

TestCustomFileModel::TestCustomFileModel() : model(nullptr), tempDir(nullptr) // <--- MODIFIED: Initialize pointer
{
    // Constructor: tempDir will be invalid here if default constructor is used.
    // It's better to ensure tempDir is valid in init() or initTestCase()
    // if path is needed before model creation.
}

TestCustomFileModel::~TestCustomFileModel()
{
    // tempDir is deleted in cleanupTestCase or if TestCustomFileModel obj itself is deleted
    // But QtTest usually calls cleanup for each test.
    // Ensuring tempDir is cleaned up.
    if (tempDir) {
        // tempDir->remove(); // remove() is automatically called by QTemporaryDir destructor
        delete tempDir;
        tempDir = nullptr;
    }
    // delete model; // Done in cleanup()
}

void TestCustomFileModel::initTestCase()
{
    qDebug() << "Starting test suite for CustomFileModel.";
    // Global setup if any (e.g. QStandardPaths for base temp location)
}

void TestCustomFileModel::cleanupTestCase()
{
    qDebug() << "Finished test suite for CustomFileModel.";
    // Global cleanup if any
}

void TestCustomFileModel::init()
{
    // Setup before each test function
    // Create a new model instance for each test to ensure independence
    // tempDir.path() will provide a unique, empty directory for each test.
    tempDir = new QTemporaryDir(); // <--- MODIFIED: Create new instance
    QVERIFY(tempDir->isValid()); // Ensure tempDir was created successfully
    originalModelRootPath = tempDir->path(); // Store the path
    model = new CustomFileModel(originalModelRootPath);
}

void TestCustomFileModel::cleanup()
{
    // Cleanup after each test function
    delete model;
    model = nullptr;
    // tempDir will be automatically removed when the TestCustomFileModel object
    // is destroyed if it's a member, or if it goes out of scope here if it were local to init().
    // For member tempDir, it's cleaned when the TestCustomFileModel instance for the current test function is destructed.

    if (tempDir) { // <--- MODIFIED: Cleanup tempDir
        // bool removed = tempDir->remove(); // remove() is automatically called by QTemporaryDir destructor
        // if (!removed) {
        //     qWarning() << "Could not remove temporary directory:" << tempDir->path()
        //                << "Error:" << tempDir->errorString();
        // }
        delete tempDir;
        tempDir = nullptr;
    }
}

void TestCustomFileModel::createPopulatedTestDirectory(const QString& basePath)
{
    QDir dir(basePath);
    QVERIFY(dir.mkpath("folderA"));
    QVERIFY(dir.mkpath("folderA/subfolderB"));
    QVERIFY(dir.mkpath("folderC"));

    QFile file1(basePath + "/file_root1.txt");
    QVERIFY(file1.open(QIODevice::WriteOnly)); file1.write("content_root1"); file1.close();

    QFile file2(basePath + "/folderA/file_A1.log");
    QVERIFY(file2.open(QIODevice::WriteOnly)); file2.write("content_A1"); file2.close();

    QFile file3(basePath + "/folderA/subfolderB/file_B1.dat");
    QVERIFY(file3.open(QIODevice::WriteOnly)); file3.write("content_B1"); file3.close();

    QFile file4(basePath + "/file_root2.txt"); // Another root file
    QVERIFY(file4.open(QIODevice::WriteOnly)); file4.write("content_root2"); file4.close();
}

// Helper function implementation
QModelIndex TestCustomFileModel::findItem(const QString& name, const QModelIndex& parent) const
{
    if (!model) return QModelIndex();
    for (int i = 0; i < model->rowCount(parent); ++i) {
        QModelIndex current = model->index(i, 0, parent);
        if (model->data(current, Qt::DisplayRole).toString() == name) {
            return current;
        }
    }
    return QModelIndex();
}

// ---- Test Cases Implementation ----

void TestCustomFileModel::testInitialState_EmptyDir()
{
    QVERIFY(model != nullptr);
    // With an empty tempDir, the model should also be empty.
    QCOMPARE(model->rowCount(QModelIndex()), 0);
}

void TestCustomFileModel::testModelConstruction_ValidPopulatedPath()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model; // Delete the one created with empty dir
    model = new CustomFileModel(originalModelRootPath); // Re-create with populated dir
    QVERIFY(model);

    // ASSERT
    // Expecting: file_root1.txt, file_root2.txt, folderA, folderC (order might vary)
    QVERIFY(model->rowCount(QModelIndex()) == 4);
    QVERIFY(model->hasFiles());
}

void TestCustomFileModel::testModelConstruction_InvalidPath()
{
    // ARRANGE
    delete model; // Delete the one from init()
    QString invalidPath = originalModelRootPath + "/non_existent_path_component_blah";
    // QTemporaryDir handles its own path, so use a path that CustomFileModel itself would resolve

    // ACT
    model = new CustomFileModel(invalidPath);
    QVERIFY(model);

    // ASSERT
    QCOMPARE(model->rowCount(QModelIndex()), 0); // Should handle gracefully
    QCOMPARE(model->hasFiles(), false);
}

void TestCustomFileModel::testColumnCount()
{
    // This test assumes the column count is fixed and known.
    // Adjust '1' if your model has more columns by default.
    QVERIFY(model->columnCount(QModelIndex()) >= 1); // Or QCOMPARE(model->columnCount(QModelIndex()), EXPECTED_COLUMN_COUNT);
}

void TestCustomFileModel::testHeaderData_DisplayRole()
{
    // ARRANGE
    // The default model from init() is fine.
    // The header should be present even if the model is empty.
    
    // ACT & ASSERT
    // Assuming column 0 is the main/only column.
    QVariant header = model->headerData(0, Qt::Horizontal, Qt::DisplayRole);
    QVERIFY(header.isValid());
    // Default Qt behavior might return "1" if not overridden.
    // Your CustomFileModel::headerData should return something meaningful e.g. "Name" or QCoreApplication::tr("Name")
    // For now, let's check it's not empty if it's being overridden.
    // QVERIFY(!header.toString().isEmpty()); // This depends on your headerData impl.
    // If you have a specific name:
    // QCOMPARE(header.toString(), QCoreApplication::tr("Name")); // Or whatever you set
    // Given the provided header, it's likely just one column.
    // If tr("Name") is in your actual customfilemodel.cpp for headerData:
    // QCOMPARE(header.toString(), QCoreApplication::tr("Name"));
    // If not, it might be an empty string or a number. For a basic check:
    QVERIFY(model->headerData(0, Qt::Horizontal, Qt::DisplayRole).isValid());
    // Test invalid section
    QVERIFY(!model->headerData(1, Qt::Horizontal, Qt::DisplayRole).isValid()); // Assuming only 1 column
}

void TestCustomFileModel::testRowCount_EmptyDir()
{
    // With an empty tempDir, the model should report 0 root items.
    QCOMPARE(model->rowCount(QModelIndex()), 0);
}

void TestCustomFileModel::testRowCount_PopulatedDir()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    // ASSERT
    // Root items: file_root1.txt, file_root2.txt, folderA, folderC
    QCOMPARE(model->rowCount(QModelIndex()), 4);

    // Find folderA (its position might vary depending on QDir::entryList sorting)
    QModelIndex folderAIndex;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIndex = current;
            break;
        }
    }
    QVERIFY(folderAIndex.isValid());
    // Items in folderA: file_A1.log, subfolderB
    QCOMPARE(model->rowCount(folderAIndex), 2);

    // Find subfolderB
    QModelIndex subfolderBIndex;
     for (int i = 0; i < model->rowCount(folderAIndex); ++i) {
        QModelIndex current = model->index(i, 0, folderAIndex);
        if (model->data(current, Qt::DisplayRole).toString() == "subfolderB") {
            subfolderBIndex = current;
            break;
        }
    }
    QVERIFY(subfolderBIndex.isValid());
    // Item in subfolderB: file_B1.dat
    QCOMPARE(model->rowCount(subfolderBIndex), 1);
}

void TestCustomFileModel::testHasFiles_EmptyDir()
{
    // With an empty tempDir, hasFiles() should be false.
    QCOMPARE(model->hasFiles(), false);
}

void TestCustomFileModel::testHasFiles_PopulatedDir()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    // ASSERT
    QVERIFY(model->hasFiles());
}

void TestCustomFileModel::testIndexAndParent_RootItems()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);
    QVERIFY(model->rowCount(QModelIndex()) > 0);

    // ACT & ASSERT
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex idx = model->index(i, 0, QModelIndex());
        QVERIFY(idx.isValid());
        QCOMPARE(idx.row(), i);
        QCOMPARE(idx.column(), 0);
        QCOMPARE(model->parent(idx), QModelIndex()); // Root items have invalid parent
    }
}

void TestCustomFileModel::testIndexAndParent_NestedItems()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    // Find folderA
    QModelIndex folderAIndex;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIndex = current;
            break;
        }
    }
    QVERIFY(folderAIndex.isValid());
    QVERIFY(model->rowCount(folderAIndex) > 0); // folderA should have children

    // ACT & ASSERT for children of folderA
    for (int i = 0; i < model->rowCount(folderAIndex); ++i) {
        QModelIndex childIdx = model->index(i, 0, folderAIndex);
        QVERIFY(childIdx.isValid());
        QCOMPARE(childIdx.row(), i);
        QCOMPARE(model->parent(childIdx), folderAIndex);

        // Check one level deeper if it's subfolderB
        if (model->data(childIdx, Qt::DisplayRole).toString() == "subfolderB") {
            QModelIndex subfolderBIndex = childIdx;
            QVERIFY(model->rowCount(subfolderBIndex) > 0);
            for (int j = 0; j < model->rowCount(subfolderBIndex); ++j) {
                QModelIndex grandchildIdx = model->index(j, 0, subfolderBIndex);
                QVERIFY(grandchildIdx.isValid());
                QCOMPARE(grandchildIdx.row(), j);
                QCOMPARE(model->parent(grandchildIdx), subfolderBIndex);
            }
        }
    }
}

void TestCustomFileModel::testData_DisplayRole()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    // ACT & ASSERT - This is a bit fragile due to sorting order of QDir::entryList
    // It's better to find items by expected name if possible.
    // Example: find "file_root1.txt"
    bool found_file_root1 = false;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex idx = model->index(i, 0, QModelIndex());
        if (model->data(idx, Qt::DisplayRole).toString() == "file_root1.txt") {
            found_file_root1 = true;
            break;
        }
    }
    QVERIFY(found_file_root1);

    // Example: find "folderA/file_A1.log"
    QModelIndex folderAIdx;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIdx = current;
            break;
        }
    }
    QVERIFY(folderAIdx.isValid());

    bool found_file_A1 = false;
    for (int i = 0; i < model->rowCount(folderAIdx); ++i) {
        QModelIndex idx = model->index(i, 0, folderAIdx);
        if (model->data(idx, Qt::DisplayRole).toString() == "file_A1.log") {
            found_file_A1 = true;
            break;
        }
    }
    QVERIFY(found_file_A1);
}

void TestCustomFileModel::testData_CheckStateRole_Initial()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);
    QVERIFY(model->rowCount(QModelIndex()) > 0);

    // ACT & ASSERT
    QModelIndex firstItemIndex = model->index(0, 0, QModelIndex());
    QVERIFY(firstItemIndex.isValid());
    QVariant checkState = model->data(firstItemIndex, Qt::CheckStateRole);
    QVERIFY(checkState.isValid());
    QCOMPARE(checkState.toInt(), Qt::Unchecked); // Assuming default is Unchecked

    // Check a nested item too
    QModelIndex folderAIndex;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIndex = current;
            break;
        }
    }
    QVERIFY(folderAIndex.isValid());
    QVERIFY(model->rowCount(folderAIndex) > 0);
    QModelIndex childOfFolderA = model->index(0,0,folderAIndex);
    QVERIFY(childOfFolderA.isValid());
    QVariant childCheckState = model->data(childOfFolderA, Qt::CheckStateRole);
    QVERIFY(childCheckState.isValid());
    QCOMPARE(childCheckState.toInt(), Qt::Unchecked);
}

void TestCustomFileModel::testFlags_IsCheckable()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);
    QVERIFY(model->rowCount(QModelIndex()) > 0);

    // ACT & ASSERT for a root file
    QModelIndex firstFileIndex; // find a file
    for(int i=0; i<model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i,0,QModelIndex());
        // Simplification: assume first item not a dir is a file.
        // A real test would ensure it finds a file. Here we assume file_root1.txt exists.
        if(model->data(current, Qt::DisplayRole).toString().contains(".txt")) {
             firstFileIndex = current;
             break;
        }
    }
    QVERIFY(firstFileIndex.isValid());
    Qt::ItemFlags flagsFile = model->flags(firstFileIndex);
    QVERIFY(flagsFile & Qt::ItemIsUserCheckable);
    QVERIFY(flagsFile & Qt::ItemIsEnabled);
    QVERIFY(flagsFile & Qt::ItemIsSelectable);


    // ACT & ASSERT for a folder
    QModelIndex folderAIndex; // find folderA
     for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIndex = current;
            break;
        }
    }
    QVERIFY(folderAIndex.isValid());
    Qt::ItemFlags flagsFolder = model->flags(folderAIndex);
    QVERIFY(flagsFolder & Qt::ItemIsUserCheckable);
    QVERIFY(flagsFolder & Qt::ItemIsEnabled);
    QVERIFY(flagsFolder & Qt::ItemIsSelectable);
}

void TestCustomFileModel::testSetData_CheckFile_EmitsSignal()
{
    // ARRANGE:
    QDir dir(originalModelRootPath);
    QFile dummyFile(dir.filePath("testfile.txt")); // Use QDir for path construction
    QVERIFY(dummyFile.open(QIODevice::WriteOnly));
    dummyFile.write("content");
    dummyFile.close();

    // Re-initialize model to pick up the new file
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);
    QCOMPARE(model->rowCount(QModelIndex()), 1); // Ensure file is loaded

    QModelIndex fileIndex = model->index(0, 0, QModelIndex());
    QVERIFY(fileIndex.isValid());
    QCOMPARE(model->data(fileIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);

    QSignalSpy spy(model, &CustomFileModel::dataChanged); // Correct signal name

    // ACT
    bool setResult = model->setData(fileIndex, Qt::Checked, Qt::CheckStateRole);

    // ASSERT
    QVERIFY(setResult);
    QCOMPARE(model->data(fileIndex, Qt::CheckStateRole).toInt(), Qt::Checked);

    QCOMPARE(spy.count(), 1); // Check that one signal was emitted
    if (spy.count() > 0) { // avoid crash if no signal
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toModelIndex(), fileIndex); // topLeft
        QCOMPARE(arguments.at(1).toModelIndex(), fileIndex); // bottomRight
        // The roles vector might contain more than just CheckStateRole if other things change too.
        // Check if it contains CheckStateRole.
        QVector<int> roles = arguments.at(2).value<QVector<int>>();
        QVERIFY(roles.contains(Qt::CheckStateRole));
    }
}

void TestCustomFileModel::testSetData_CheckFolder_Propagation_EmitsSignal()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    QModelIndex folderAIndex = findItem("folderA");
    QVERIFY(folderAIndex.isValid());

    // CustomFileModel sorts DirsFirst, then by Name.
    // Inside folderA: subfolderB (dir), file_A1.log (file)
    QModelIndex subfolderBIndex = findItem("subfolderB", folderAIndex);
    QModelIndex fileA1Index = findItem("file_A1.log", folderAIndex);
    QVERIFY(subfolderBIndex.isValid());
    QVERIFY(fileA1Index.isValid());
    // Verify we found the correct items
    QCOMPARE(model->data(subfolderBIndex, Qt::DisplayRole).toString(), "subfolderB");
    QCOMPARE(model->data(fileA1Index, Qt::DisplayRole).toString(), "file_A1.log");


    QModelIndex fileB1Index = findItem("file_B1.dat", subfolderBIndex);
    QVERIFY(fileB1Index.isValid());
    QCOMPARE(model->data(fileB1Index, Qt::DisplayRole).toString(), "file_B1.dat");


    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(model->data(subfolderBIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(model->data(fileB1Index, Qt::CheckStateRole).toInt(), Qt::Unchecked);

    QSignalSpy spy(model, &CustomFileModel::dataChanged);

    // ACT
    bool setResult = model->setData(folderAIndex, Qt::Checked, Qt::CheckStateRole);

    // ASSERT
    QVERIFY(setResult);
    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Checked);     
    QCOMPARE(model->data(subfolderBIndex, Qt::CheckStateRole).toInt(), Qt::Checked);  
    QCOMPARE(model->data(fileB1Index, Qt::CheckStateRole).toInt(), Qt::Checked);      

    QVERIFY(spy.count() >= 1); 
    bool folderAChangedInSignal = false;
    // Use index-based loop for QSignalSpy
    for (int i = 0; i < spy.count(); ++i) {
        const QList<QVariant>& sigArgs = spy.at(i);
        QModelIndex topLeft = sigArgs.at(0).toModelIndex();
        QModelIndex bottomRight = sigArgs.at(1).toModelIndex();
        QVector<int> roles = sigArgs.at(2).value<QVector<int>>();
        if (!roles.contains(Qt::CheckStateRole)) continue;

        if (topLeft == folderAIndex && bottomRight == folderAIndex) {
            folderAChangedInSignal = true;
            // No need to break, let's check all signals if needed for debug, but one is enough for verify.
        }
    }
    QVERIFY(folderAChangedInSignal); 
}

void TestCustomFileModel::testSetData_UncheckFile_ParentUpdate_EmitsSignal()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model;
    model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    QModelIndex folderAIndex = findItem("folderA");
    QVERIFY(folderAIndex.isValid());
    
    // Set folderA and all its children to Checked initially
    QVERIFY(model->setData(folderAIndex, Qt::Checked, Qt::CheckStateRole));

    // CustomFileModel sorts DirsFirst. In folderA: subfolderB, file_A1.log
    QModelIndex subfolderBIndex = findItem("subfolderB", folderAIndex);
    QModelIndex fileA1Index = findItem("file_A1.log", folderAIndex);
    QModelIndex fileB1Index = findItem("file_B1.dat", subfolderBIndex); 
    QVERIFY(subfolderBIndex.isValid());
    QVERIFY(fileA1Index.isValid());
    QVERIFY(fileB1Index.isValid());

    // Verify initial states after checking folderA
    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(model->data(subfolderBIndex, Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(model->data(fileB1Index, Qt::CheckStateRole).toInt(), Qt::Checked); // This was the failing one

    QSignalSpy spy(model, &CustomFileModel::dataChanged);

    // ACT: Uncheck file_A1.log
    bool setResult = model->setData(fileA1Index, Qt::Unchecked, Qt::CheckStateRole);

    // ASSERT
    QVERIFY(setResult);
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::PartiallyChecked); 
    QCOMPARE(model->data(subfolderBIndex, Qt::CheckStateRole).toInt(), Qt::Checked); // Should remain checked
    QCOMPARE(model->data(fileB1Index, Qt::CheckStateRole).toInt(), Qt::Checked);   // Should remain checked

    QVERIFY(spy.count() >= 1); 
    bool fileA1ChangedInSignal = false;
    bool folderAUpdatedInSignal = false;
    // Use index-based loop for QSignalSpy
    for (int i = 0; i < spy.count(); ++i) {
        const QList<QVariant>& sigArgs = spy.at(i);
        QModelIndex topLeft = sigArgs.at(0).toModelIndex();
        QModelIndex bottomRight = sigArgs.at(1).toModelIndex();
        QVector<int> roles = sigArgs.at(2).value<QVector<int>>();
        if (!roles.contains(Qt::CheckStateRole)) continue;

        if (topLeft == fileA1Index && bottomRight == fileA1Index) fileA1ChangedInSignal = true;
        if (topLeft == folderAIndex && bottomRight == folderAIndex) folderAUpdatedInSignal = true;
    }
    QVERIFY(fileA1ChangedInSignal);
    QVERIFY(folderAUpdatedInSignal);
}

void TestCustomFileModel::testToggleCheckState_File()
{
    // ARRANGE
    QDir dir(originalModelRootPath);
    QFile dummyFile(dir.filePath("toggle_test.txt"));
    QVERIFY(dummyFile.open(QIODevice::WriteOnly)); dummyFile.close();
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model && model->rowCount(QModelIndex()) == 1);
    QModelIndex fileIndex = model->index(0,0,QModelIndex());
    QCOMPARE(model->data(fileIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);

    // ACT 1: Unchecked -> Checked
    model->toggleCheckState(fileIndex);
    // ASSERT 1
    QCOMPARE(model->data(fileIndex, Qt::CheckStateRole).toInt(), Qt::Checked);

    // ACT 2: Checked -> Unchecked
    model->toggleCheckState(fileIndex);
    // ASSERT 2
    QCOMPARE(model->data(fileIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
}

void TestCustomFileModel::testToggleCheckState_Folder_Propagation()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QModelIndex folderAIndex;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIndex = current; break; }
    }
    QVERIFY(folderAIndex.isValid());
    QModelIndex fileA1Index = model->index(0,0,folderAIndex);
    QVERIFY(fileA1Index.isValid());

    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Unchecked);

    // ACT 1: Toggle folderA (Unchecked -> Checked)
    model->toggleCheckState(folderAIndex);
    // ASSERT 1
    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Checked); // Propagated

    // ACT 2: Toggle folderA again (Checked -> Unchecked)
    model->toggleCheckState(folderAIndex);
    // ASSERT 2
    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Unchecked); // Propagated
}

void TestCustomFileModel::testSetAllCheckStates_Checked()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model->rowCount(QModelIndex()) > 0); // Ensure model is populated

    // ACT
    model->setAllCheckStates(Qt::Checked);

    // ASSERT
    // Check a root file
    QModelIndex rootFileIndex = findItem("file_root1.txt"); 
    QVERIFY(rootFileIndex.isValid());
    QCOMPARE(model->data(rootFileIndex, Qt::CheckStateRole).toInt(), Qt::Checked);
    
    // Check a folder
    QModelIndex folderAIndex = findItem("folderA");
    QVERIFY(folderAIndex.isValid());
    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Checked);

    // Check a nested file within that folder
    QModelIndex fileA1Index = findItem("file_A1.log", folderAIndex); 
    QVERIFY(fileA1Index.isValid());
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Checked);

    // Check a deeply nested file
    QModelIndex subfolderBIndex = findItem("subfolderB", folderAIndex); 
    QVERIFY(subfolderBIndex.isValid());
    QModelIndex fileB1Index = findItem("file_B1.dat", subfolderBIndex); 
    QVERIFY(fileB1Index.isValid()); // This was the failing point
    QCOMPARE(model->data(fileB1Index, Qt::CheckStateRole).toInt(), Qt::Checked);
}

void TestCustomFileModel::testSetAllCheckStates_Unchecked()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model->rowCount(QModelIndex()) > 0);
    // First, set all to checked
    model->setAllCheckStates(Qt::Checked);
    QModelIndex rootFileIndex = model->index(0,0,QModelIndex());
    QCOMPARE(model->data(rootFileIndex, Qt::CheckStateRole).toInt(), Qt::Checked); // Verify pre-condition

    // ACT
    model->setAllCheckStates(Qt::Unchecked);

    // ASSERT
    QCOMPARE(model->data(rootFileIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QModelIndex folderAIndex;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIndex = current; break; }
    }
    QVERIFY(folderAIndex.isValid());
    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QModelIndex fileA1Index = model->index(0,0,folderAIndex);
    QVERIFY(fileA1Index.isValid());
    QCOMPARE(model->data(fileA1Index, Qt::CheckStateRole).toInt(), Qt::Unchecked);
}

// ---- Path Retrieval Tests ----
void TestCustomFileModel::testGetCheckedFilesPaths_NoneChecked()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    // ACT
    QStringList paths = model->getCheckedFilesPaths();

    // ASSERT
    QVERIFY(paths.isEmpty());
}

void TestCustomFileModel::testGetCheckedFilesPaths_OnlyFilesChecked()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    // Find and check file_root1.txt and folderA/subfolderB/file_B1.dat
    QModelIndex fileRoot1Index, fileB1Index;
    QModelIndex folderAIndex, subfolderBIndex;

    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "file_root1.txt") {
            fileRoot1Index = current;
        } else if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIndex = current;
        }
    }
    QVERIFY(fileRoot1Index.isValid());
    QVERIFY(folderAIndex.isValid());

    for (int i = 0; i < model->rowCount(folderAIndex); ++i) {
        QModelIndex current = model->index(i, 0, folderAIndex);
        if (model->data(current, Qt::DisplayRole).toString() == "subfolderB") {
            subfolderBIndex = current;
            break;
        }
    }
    QVERIFY(subfolderBIndex.isValid());
    fileB1Index = model->index(0, 0, subfolderBIndex); // file_B1.dat
    QVERIFY(fileB1Index.isValid() && model->data(fileB1Index, Qt::DisplayRole).toString() == "file_B1.dat");

    model->setData(fileRoot1Index, Qt::Checked, Qt::CheckStateRole);
    model->setData(fileB1Index, Qt::Checked, Qt::CheckStateRole);
    // Ensure folderA and subfolderB are not fully checked, but partially if they become so
    QCOMPARE(model->data(fileRoot1Index, Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(model->data(fileB1Index, Qt::CheckStateRole).toInt(), Qt::Checked);
    QVERIFY(model->data(folderAIndex, Qt::CheckStateRole).toInt() == Qt::PartiallyChecked || model->data(folderAIndex, Qt::CheckStateRole).toInt() == Qt::Unchecked) ; // Depending on how deep unchecking affects parents
    QVERIFY(model->data(subfolderBIndex, Qt::CheckStateRole).toInt() == Qt::Checked || model->data(subfolderBIndex, Qt::CheckStateRole).toInt() == Qt::PartiallyChecked) ; // fileB1Index is its only child


    // ACT
    QStringList paths = model->getCheckedFilesPaths();

    // ASSERT
    QCOMPARE(paths.count(), 2);
    // Paths should be absolute
    QString expectedPathRoot1 = QDir(originalModelRootPath).filePath("file_root1.txt");
    QString expectedPathB1 = QDir(originalModelRootPath).filePath("folderA/subfolderB/file_B1.dat");
    QVERIFY(paths.contains(expectedPathRoot1));
    QVERIFY(paths.contains(expectedPathB1));
}

void TestCustomFileModel::testGetCheckedFilesPaths_FolderChecked()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    QModelIndex folderAIndex;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "folderA") {
            folderAIndex = current; break; }
    }
    QVERIFY(folderAIndex.isValid());

    // Check folderA (this should propagate to children: file_A1.log, subfolderB, and file_B1.dat)
    model->setData(folderAIndex, Qt::Checked, Qt::CheckStateRole);
    QCOMPARE(model->data(folderAIndex, Qt::CheckStateRole).toInt(), Qt::Checked);

    // ACT
    QStringList paths = model->getCheckedFilesPaths();

    // ASSERT
    // Expected: file_A1.log, file_B1.dat (files within folderA, recursively)
    QCOMPARE(paths.count(), 2);
    QString expectedPathA1 = QDir(originalModelRootPath).filePath("folderA/file_A1.log");
    QString expectedPathB1 = QDir(originalModelRootPath).filePath("folderA/subfolderB/file_B1.dat");
    QVERIFY(paths.contains(expectedPathA1));
    QVERIFY(paths.contains(expectedPathB1));
}

void TestCustomFileModel::testGetCheckedFilesPaths_MixedContent()
{
    // ARRANGE
    createPopulatedTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    QModelIndex fileRoot1Index, folderAIndex, folderCIndex;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        QString name = model->data(current, Qt::DisplayRole).toString();
        if (name == "file_root1.txt") fileRoot1Index = current;
        else if (name == "folderA") folderAIndex = current;
        else if (name == "folderC") folderCIndex = current; // folderC is empty
    }
    QVERIFY(fileRoot1Index.isValid());
    QVERIFY(folderAIndex.isValid());
    QVERIFY(folderCIndex.isValid());

    // Check file_root1.txt
    model->setData(fileRoot1Index, Qt::Checked, Qt::CheckStateRole);
    // Check folderA (propagates to file_A1.log, subfolderB, file_B1.dat)
    model->setData(folderAIndex, Qt::Checked, Qt::CheckStateRole);
    // Check folderC (which is empty, so it adds no file paths)
    model->setData(folderCIndex, Qt::Checked, Qt::CheckStateRole);

    // ACT
    QStringList paths = model->getCheckedFilesPaths();

    // ASSERT
    // Expected: file_root1.txt, file_A1.log, file_B1.dat
    QCOMPARE(paths.count(), 3);
    QString expectedPathRoot1 = QDir(originalModelRootPath).filePath("file_root1.txt");
    QString expectedPathA1 = QDir(originalModelRootPath).filePath("folderA/file_A1.log");
    QString expectedPathB1 = QDir(originalModelRootPath).filePath("folderA/subfolderB/file_B1.dat");
    QVERIFY(paths.contains(expectedPathRoot1));
    QVERIFY(paths.contains(expectedPathA1));
    QVERIFY(paths.contains(expectedPathB1));
}

// ---- Extension-based Selection Tests ----
void TestCustomFileModel::createExtensionTestDirectory(const QString& basePath)
{
    QDir dir(basePath);
    QVERIFY(dir.mkpath("subfolder1"));
    QVERIFY(dir.mkpath("subfolder2/empty_sub"));

    auto createFile = [&](const QString& name, const QString& content = "dummy"){
        QFile file(basePath + "/" + name);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content.toUtf8());
        file.close();
    };

    createFile("file.txt");
    createFile("doc.log");
    createFile("image.TXT"); // Different case
    createFile("data.dat");
    createFile("archive.zip");
    createFile("subfolder1/another.txt");
    createFile("subfolder1/script.sh");
    createFile("subfolder1/config.LOG"); // Different case
    createFile("subfolder2/old_doc.log");
    createFile("subfolder2/text_file.txt");
}

void TestCustomFileModel::testSelectFilesByExtension_SpecificFolder_NoRecursion()
{
    // ARRANGE
    createExtensionTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    QModelIndex rootIndex = QModelIndex(); 

    model->selectFilesByExtension(rootIndex, ".txt");

    // ASSERT
    QStringList checkedPathsFoundInLoop; // Renamed for clarity, though getCheckedFilesPaths is primary validation
    
    for (int i = 0; i < model->rowCount(rootIndex); ++i) {
        QModelIndex itemIndex = model->index(i, 0, rootIndex);
        QString itemName = model->data(itemIndex, Qt::DisplayRole).toString();
        bool isChecked = model->data(itemIndex, Qt::CheckStateRole).toInt() == Qt::Checked;
        if (itemName == "file.txt") QVERIFY(isChecked);
        else if (itemName == "image.TXT") QVERIFY(isChecked);
        else if (itemName == "subfolder1") { 
            QModelIndex subfolder1Index = itemIndex;
            QModelIndex anotherTxtIndex;
            for(int j=0; j < model->rowCount(subfolder1Index); ++j) {
                QModelIndex child = model->index(j,0,subfolder1Index);
                if(model->data(child, Qt::DisplayRole).toString() == "another.txt") {
                    anotherTxtIndex = child;
                    break;
                }
            }
            QVERIFY(anotherTxtIndex.isValid());
            QVERIFY(model->data(anotherTxtIndex, Qt::CheckStateRole).toInt() == Qt::Unchecked);
        }
        // else QVERIFY(!isChecked); // This can be too strict if other non-txt files exist or folders become partially checked.
                                 // Rely on getCheckedFilesPaths for overall correctness of *file* selection.

        if(isChecked && !QFileInfo(QDir(originalModelRootPath).filePath(itemName)).isDir()) {
             checkedPathsFoundInLoop.append(QDir(originalModelRootPath).filePath(itemName));
        }
    }
    
    QStringList allChecked = model->getCheckedFilesPaths();
    QCOMPARE(allChecked.count(), 2);
    QVERIFY(allChecked.contains(QDir(originalModelRootPath).filePath("file.txt")));
    QVERIFY(allChecked.contains(QDir(originalModelRootPath).filePath("image.TXT")));

    model->setAllCheckStates(Qt::Unchecked); 
    model->selectFilesByExtension(rootIndex, ".nonexistent");
    QCOMPARE(model->getCheckedFilesPaths().count(), 0);
}

void TestCustomFileModel::testSelectFilesByExtensionRecursive_FromRoot()
{
    // ARRANGE
    createExtensionTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    QModelIndex rootIndex = QModelIndex();

    // ACT: Select .log files recursively from root
    model->selectFilesByExtensionRecursive(rootIndex, ".log");

    // ASSERT
    // Expected checked: doc.log, subfolder1/config.LOG, subfolder2/old_doc.log
    QStringList paths = model->getCheckedFilesPaths();
    QCOMPARE(paths.count(), 3);
    QDir baseDir(originalModelRootPath);
    QVERIFY(paths.contains(baseDir.filePath("doc.log")));
    QVERIFY(paths.contains(baseDir.filePath("subfolder1/config.LOG")));
    QVERIFY(paths.contains(baseDir.filePath("subfolder2/old_doc.log")));

    // Verify other files are not checked, e.g. file.txt
    QModelIndex fileTxtIndex;
     for (int i = 0; i < model->rowCount(rootIndex); ++i) {
        QModelIndex current = model->index(i, 0, rootIndex);
        if (model->data(current, Qt::DisplayRole).toString() == "file.txt") {
            fileTxtIndex = current;
            break;
        }
    }
    QVERIFY(fileTxtIndex.isValid());
    QCOMPARE(model->data(fileTxtIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
}

void TestCustomFileModel::testSelectFilesByExtensionRecursive_FromSubfolder()
{
    // ARRANGE
    createExtensionTestDirectory(originalModelRootPath);
    delete model; model = new CustomFileModel(originalModelRootPath);
    QVERIFY(model);

    QModelIndex subfolder1Index;
    for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "subfolder1") {
            subfolder1Index = current;
            break;
        }
    }
    QVERIFY(subfolder1Index.isValid());

    // ACT: Select .txt files recursively from subfolder1
    model->selectFilesByExtensionRecursive(subfolder1Index, ".txt");

    // ASSERT
    // Expected checked: subfolder1/another.txt
    // Expected UNCHECKED: file.txt, image.TXT (root), subfolder2/text_file.txt
    QStringList paths = model->getCheckedFilesPaths();
    QCOMPARE(paths.count(), 1);
    QDir baseDir(originalModelRootPath);
    QVERIFY(paths.contains(baseDir.filePath("subfolder1/another.txt")));

    // Verify root .txt files are NOT checked
    QModelIndex fileTxtIndex;
     for (int i = 0; i < model->rowCount(QModelIndex()); ++i) {
        QModelIndex current = model->index(i, 0, QModelIndex());
        if (model->data(current, Qt::DisplayRole).toString() == "file.txt") {
            fileTxtIndex = current;
            break;
        }
    }
    QVERIFY(fileTxtIndex.isValid());
    QCOMPARE(model->data(fileTxtIndex, Qt::CheckStateRole).toInt(), Qt::Unchecked);
}


// QTEST_APPLESS_MAIN(TestCustomFileModel) // Can be used if no GUI, no event loop needed for tests
QTEST_MAIN(TestCustomFileModel) // Or QTEST_GUILESS_MAIN if some Qt features need an event loop but no GUI
// Using QTEST_MAIN for broader compatibility in case event loop is needed by some model functions.

#include "tst_customfilemodel.moc" // Required for MOC to process the Q_OBJECT 