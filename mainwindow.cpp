// mainwindow.cpp

#include "mainwindow.h"
#include "customfilemodel.h"
#include "filemergerlogic.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
// #include <QGridLayout> // Not strictly needed based on current layout
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QProgressBar>
#include <QHeaderView>  // For fileTreeView->header()
#include <QGroupBox>    // For QGroupBox

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), progressBar(nullptr), fileModel(nullptr), mergerLogic(nullptr), currentFolderPath("")
{
    // Basic window setup
    setWindowTitle(tr("文件合并工具 (File Merger Tool)"));
    resize(800, 600);

    // Central widget and layout
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 1. Source Folder Selection
    QGroupBox *folderGroup = new QGroupBox(tr("源文件夹选择 (Source Folder Selection)"));
    QHBoxLayout *folderLayout = new QHBoxLayout(folderGroup);
    folderLayout->addWidget(new QLabel(tr("请选择要处理的文件夹 (Select folder to process):")));
    folderPathLineEdit = new QLineEdit();
    folderPathLineEdit->setReadOnly(true);
    folderLayout->addWidget(folderPathLineEdit, 1); // Stretch factor 1
    browseButton = new QPushButton(tr("浏览文件夹 (Browse)"));
    folderLayout->addWidget(browseButton);
    mainLayout->addWidget(folderGroup);

    // 2. File Selection
    QGroupBox *fileSelectGroup = new QGroupBox(tr("文件选择 (File Selection)"));
    QVBoxLayout *fileSelectLayout = new QVBoxLayout(fileSelectGroup);
    fileTreeView = new QTreeView();
    fileSelectLayout->addWidget(fileTreeView);
    mainLayout->addWidget(fileSelectGroup, 1); // Stretch factor 1 for this group

    // 3. Actions & Output
    QGroupBox *actionGroup = new QGroupBox(tr("操作与输出 (Actions & Output)"));
    QHBoxLayout *actionLayout = new QHBoxLayout(actionGroup);
    selectAllButton = new QPushButton(tr("全选 (Select All)"));
    deselectAllButton = new QPushButton(tr("全不选 (Deselect All)"));
    mergeButton = new QPushButton(tr("开始合并 (Start Merge)"));
    actionLayout->addWidget(selectAllButton);
    actionLayout->addWidget(deselectAllButton);
    actionLayout->addStretch();
    actionLayout->addWidget(mergeButton);
    mainLayout->addWidget(actionGroup);

    // Status Bar (QMainWindow has one by default)
    statusBar = QMainWindow::statusBar(); // Get the default status bar
    progressBar = new QProgressBar(statusBar);
    statusBar->addPermanentWidget(progressBar);
    progressBar->hide(); // Initially hidden
    progressBar->setRange(0,100); // Default progress range
    progressBar->setTextVisible(false); // Or true if you want to show percentage text on bar

    updateStatus(tr("请选择一个文件夹 (Please select a folder)."));

    // Initialize logic and model
    mergerLogic = new FileMergerLogic(this); // Parent it to MainWindow for auto-deletion

    // Initial button states
    selectAllButton->setEnabled(false);
    deselectAllButton->setEnabled(false);
    mergeButton->setEnabled(false);

    connectSignalsAndSlots();
}

MainWindow::~MainWindow()
{
    // mergerLogic is parented, Qt handles its deletion
    // fileModel is also parented to MainWindow if set, or should be deleted if not
    // No need for the custom deletion logic for fileModel if it's always parented
    // if (fileModel && !fileModel->parent()) {
    //     delete fileModel;
    // }
    // progressBar is a child of statusBar, will be deleted with it.
}

void MainWindow::connectSignalsAndSlots()
{
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::browseFolder);
    connect(fileTreeView, &QTreeView::clicked, this, &MainWindow::onTreeViewClicked);
    connect(selectAllButton, &QPushButton::clicked, this, &MainWindow::selectAllFiles);
    connect(deselectAllButton, &QPushButton::clicked, this, &MainWindow::deselectAllFiles);
    connect(mergeButton, &QPushButton::clicked, this, &MainWindow::startMerge);

    // Connect signals from FileMergerLogic
    connect(mergerLogic, &FileMergerLogic::statusUpdated, this, &MainWindow::updateStatus);
    connect(mergerLogic, &FileMergerLogic::mergeFinished, this, &MainWindow::mergeProcessFinished);
    connect(mergerLogic, &FileMergerLogic::progressUpdated, this, &MainWindow::updateProgressBar);
}


void MainWindow::browseFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择文件夹 (Select Folder)"),
                                                     currentFolderPath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation) : currentFolderPath,
                                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        currentFolderPath = dir;
        folderPathLineEdit->setText(currentFolderPath);
        updateStatus(tr("正在加载文件列表... (Loading file list...)"));

        if (fileModel) {
            fileTreeView->setModel(nullptr); // Disconnect old model
            delete fileModel; // Delete old model
            fileModel = nullptr;
        }
        fileModel = new CustomFileModel(currentFolderPath, this); // Parent to MainWindow
        fileTreeView->setModel(fileModel);
        // fileTreeView->expandAll(); // Optionally expand all items
        fileTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);


        bool filesFound = fileModel->hasFiles();
        selectAllButton->setEnabled(filesFound);
        deselectAllButton->setEnabled(filesFound);
        mergeButton->setEnabled(filesFound);

        if (filesFound) {
            updateStatus(tr("文件列表已加载。请选择文件。 (File list loaded. Please select files.)"));
        } else {
            updateStatus(tr("在选定文件夹中未找到文件。 (No files found in the selected folder.)"));
        }
    }
}

void MainWindow::onTreeViewClicked(const QModelIndex &index)
{
    if (fileModel && index.isValid()) {
        // fileModel->toggleCheckState(index); // Let the view handle it directly via setData
        // The model should emit dataChanged, and the view will update automatically.
    }
    // Enable/disable merge button based on whether any item is checked
}

void MainWindow::selectAllFiles()
{
    if (fileModel) {
        fileModel->setAllCheckStates(Qt::Checked);
    }
}

void MainWindow::deselectAllFiles()
{
    if (fileModel) {
        fileModel->setAllCheckStates(Qt::Unchecked);
    }
}

void MainWindow::startMerge()
{
    if (!fileModel) {
        QMessageBox::warning(this, tr("错误 (Error)"), tr("请先加载一个文件夹。 (Please load a folder first.)"));
        return;
    }

    QStringList filesToMerge = fileModel->getCheckedFilesPaths();
    if (filesToMerge.isEmpty()) {
        QMessageBox::information(this, tr("未选择文件 (No Files Selected)"), tr("请至少选择一个文件进行合并。 (Please select at least one file to merge.)"));
        return;
    }

    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) {
        desktopPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation); // Fallback
        if (desktopPath.isEmpty()) { // Further fallback if home is not writable/found
             desktopPath = "."; // Current directory
             QMessageBox::information(this, tr("提示 (Info)"), tr("无法找到桌面或用户主目录，文件将保存到当前应用目录。 (Desktop or home path not found. File will be saved to current application directory.)"));
        } else {
            QMessageBox::information(this, tr("提示 (Info)"), tr("无法找到桌面路径，文件将保存到用户主目录。 (Desktop path not found. File will be saved to home directory.)"));
        }
    }

    // Disable buttons during merge
    mergeButton->setEnabled(false);
    browseButton->setEnabled(false);
    selectAllButton->setEnabled(false);
    deselectAllButton->setEnabled(false);
    fileTreeView->setEnabled(false); // Disable tree view as well

    updateStatus(tr("正在合并文件... (Merging files...)"));
    progressBar->setValue(0);
    progressBar->show();
    mergerLogic->startMergeProcess(filesToMerge, desktopPath);
}

void MainWindow::updateStatus(const QString &message)
{
    statusBar->showMessage(message);
}

void MainWindow::mergeProcessFinished(bool success, const QString &messageOrPath)
{
    // Re-enable buttons
    mergeButton->setEnabled(fileModel ? fileModel->hasFiles() : false);
    browseButton->setEnabled(true);
    selectAllButton->setEnabled(fileModel ? fileModel->hasFiles() : false);
    deselectAllButton->setEnabled(fileModel ? fileModel->hasFiles() : false);
    fileTreeView->setEnabled(true);
    progressBar->hide();


    if (success) {
        QMessageBox::information(this, tr("合并完成 (Merge Complete)"), tr("文件合并成功！已保存到: (Files merged successfully! Saved to:) ") + messageOrPath);
        updateStatus(tr("合并完成。 (Merge complete.) ") + tr("文件已保存到: (File saved to:) ") + messageOrPath);
    } else {
        QMessageBox::critical(this, tr("合并失败 (Merge Failed)"), messageOrPath);
        updateStatus(tr("合并失败: (Merge failed:) ") + messageOrPath);
    }
}

void MainWindow::updateProgressBar(int value) {
    if (value >= 0 && value <= 100) {
        progressBar->setValue(value);
        if (value == 0 || value == 100) { // Hide if reset or complete from logic's perspective
             // progressBar->hide(); // Let mergeProcessFinished handle hiding
        } else {
            progressBar->show();
        }
    }
} 