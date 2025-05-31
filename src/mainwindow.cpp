// mainwindow.cpp

#include "mainwindow.h"
#include "customfilemodel.h"
#include "filemergerlogic.h"
#include "treeitem.h"      // Added for TreeItem
#include <QDebug>          // Added for qDebug
#include <QFileInfo>       // Added for QFileInfo
#include <QtAlgorithms>    // Added for qSort (though often pulled in by other headers)
#include <algorithm>     // Added for std::sort
#include <QMenuBar>        // Added to provide full definition for QMenuBar

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
#include <QInputDialog> // For QInputDialog
#include <QMenu>        // For QMenu (already included, but good for context)

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
    fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu); // Set context menu policy
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

    // Create a Tools menu
    QMenu *toolsMenu = menuBar()->addMenu(tr("工具 (&T)"));

    // Add new action for recursive selection by extension
    actionRecursiveSelectByExtension = new QAction(tr("递归按后缀选择... (&R)"), this);
    toolsMenu->addAction(actionRecursiveSelectByExtension);

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

    // Connect context menu signal
    connect(fileTreeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);

    // Connect new action signal
    connect(actionRecursiveSelectByExtension, &QAction::triggered, this, &MainWindow::onRecursiveSelectByExtensionTriggered);
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

void MainWindow::showContextMenu(const QPoint &point)
{
    QModelIndex index = fileTreeView->indexAt(point);
    if (!index.isValid()) {
        return;
    }

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if (!item || item->type() != TreeItem::Folder) {
        return;
    }

    // Optional: Restrict to root folders if desired
    // if (index.parent().isValid()) { // This means it's not a direct child of the invisible root
    //     qDebug() << "Context menu requested on non-root folder:" << item->name();
    //     return; // Only for root folders (direct children of currentFolderPath)
    // }
    // For now, allowing on any folder. If only root folders, the check above `index.parent().isValid()`
    // would be `if (index.parent() != fileModel->index(fileModel->rowCount(QModelIndex())-1,0,QModelIndex()))`
    // or simpler, check TreeItem's parent.
    // A TreeItem is a root folder if its parent is the model's conceptual rootItem.
    // TreeItem* parentOfClickedItem = item->parentItem();
    // if (parentOfClickedItem == nullptr || parentOfClickedItem->parentItem() != nullptr) {
    //     // This logic depends on how CustomFileModel's rootItem is structured.
    //     // Assuming CustomFileModel::index() returns QModelIndex() for parent of top-level items.
    //     // So, if index.parent().isValid() is false, it's a top-level item.
    //     // The requirement was "root folder (direct child of fileModel->rootItem)".
    //     // This means index.parent() should be invalid.
    //     // The current CustomFileModel structure has an invisible rootItem.
    //     // Folders displayed in the tree are children of this invisible rootItem,
    //     // or children of other displayed folders.
    //     // So, a "root folder" in the display is one whose parent in the model is the invisible root.
    //     // The QModelIndex for the invisible root is QModelIndex().
    //     // Thus, if index.parent() == QModelIndex(), it's a "root folder" in the display.
    //     // QModelIndex::parent() returns an invalid QModelIndex for top-level items.
    // }
    // For this tool, any folder can show this context menu for its direct file children.

    QMenu contextMenu(this);
    contextMenu.setObjectName("FileTreeContextMenu"); // Set object name for testing/styling

    QStringList extensions;
    TreeItem* folderItem = item; // Already have the item

    for (int i = 0; i < folderItem->childCount(); ++i) {
        TreeItem* childItem = folderItem->child(i);
        if (childItem && childItem->type() == TreeItem::File) {
            QFileInfo fileInfo(childItem->name());
            QString ext = fileInfo.suffix(); // suffix() returns "txt" from "file.txt"
            if (!ext.isEmpty()) {
                QString extWithDot = "." + ext;
                if (!extensions.contains(extWithDot, Qt::CaseInsensitive)) {
                    extensions.append(extWithDot);
                }
            }
        }
    }
    // New approach for Qt 6 using std::sort and a lambda for case-insensitive sorting:
    std::sort(extensions.begin(), extensions.end(), [](const QString &s1, const QString &s2) {
        return s1.compare(s2, Qt::CaseInsensitive) < 0;
    });

    if (extensions.isEmpty()) {
        QAction* noFilesAction = contextMenu.addAction(tr("No file extensions found in this folder"));
        noFilesAction->setEnabled(false);
    } else {
        for (const QString &ext : extensions) {
            QAction *action = contextMenu.addAction(tr("Select all *%1 files").arg(ext));
            // Using lambda to capture necessary data (folderIndex and extension)
            connect(action, &QAction::triggered, this, [this, index, ext]() {
                this->handleSelectByExtensionTriggered(index, ext);
            });
        }
    }

    contextMenu.exec(fileTreeView->viewport()->mapToGlobal(point));
}

void MainWindow::handleSelectByExtensionTriggered(const QModelIndex& folderIndex, const QString& extension)
{
    qDebug() << "handleSelectByExtensionTriggered: Folder" << folderIndex.data().toString() << "Extension" << extension;
    if (fileModel && folderIndex.isValid()) {
        // The extension passed here includes the dot, e.g., ".txt"
        // The model's selectFilesByExtension expects this format or handles it.
        // (Checked customfilemodel.cpp, it prepends "." if missing, so sending ".txt" is fine)
        fileModel->selectFilesByExtension(folderIndex, extension);
    }
}

void MainWindow::onRecursiveSelectByExtensionTriggered()
{
    if (!fileModel) {
        QMessageBox::information(this, tr("无模型 (No Model)"), tr("请先加载一个文件夹。 (Please load a folder first.)"));
        return;
    }

    bool ok;
    QString extension = QInputDialog::getText(this, tr("按后缀选择 (Select by Extension)"),
                                              tr("请输入文件后缀名 (例如 .txt, log): (Enter file extension (e.g., .txt, log):)"),
                                              QLineEdit::Normal, "", &ok);

    if (ok && !extension.isEmpty()) {
        QModelIndex targetStartIndex = fileTreeView->currentIndex();
        TreeItem* selectedItem = nullptr;
        bool useSelectedFolder = false;

        if (targetStartIndex.isValid()) {
            selectedItem = static_cast<TreeItem*>(targetStartIndex.internalPointer());
            if (selectedItem && selectedItem->type() == TreeItem::Folder) {
                useSelectedFolder = true;
            }
        }

        QModelIndex effectiveStartIndex;
        QString operationScopeMessage;
        if (useSelectedFolder) {
            effectiveStartIndex = targetStartIndex;
            operationScopeMessage = tr("在文件夹 '%1' 中 (In folder '%1')").arg(selectedItem->name());
        } else {
            effectiveStartIndex = QModelIndex(); // Root
            operationScopeMessage = tr("在根目录中 (In root directory)");
        }

        qDebug() << "Recursive select by extension triggered." << operationScopeMessage << "Extension:" << extension;
        updateStatus(tr("正在按后缀 '%1' %2 选择文件... (Selecting files by extension '%1' %2...)").arg(extension, operationScopeMessage));

        fileModel->selectFilesByExtensionRecursive(effectiveStartIndex, extension);

        // The model changes should trigger view updates and status updates via updateFolderCheckState.
        // We can set a general status update here too.
        updateStatus(tr("按后缀 '%1' %2 选择操作完成。 (Selection by extension '%1' %2 complete.)").arg(extension, operationScopeMessage));

    } else if (ok && extension.isEmpty()){
        QMessageBox::warning(this, tr("输入无效 (Invalid Input)"), tr("后缀名不能为空。 (Extension cannot be empty.)"));
    }
}