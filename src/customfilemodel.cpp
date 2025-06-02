#include "customfilemodel.h"
#include "treeitem.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <functional>   // Required for std::function
#include <QDirIterator>
#include <QMimeDatabase>
#include <QMimeType>

CustomFileModel::CustomFileModel(const QString &rootPath, QObject *parent)
    : QAbstractItemModel(parent)
{
    // The rootItem in QAbstractItemModel is conceptual (QModelIndex()).
    // We create our own TreeItem that acts as the invisible root for our data.
    // The actual displayed root items will be children of this conceptual rootItem.
    rootItem = new TreeItem(QStringLiteral("__InvisibleRoot__"), TreeItem::Folder, nullptr); // Use new constructor
    setupModelData(rootPath, rootItem);

    // Example: Filter for specific file types - adjust as needed
    // nameFilters << "*.txt" << "*.log"; // If you want to filter
    // If nameFilters is empty, all files (matching QDir::Files) will be listed.
}

CustomFileModel::~CustomFileModel()
{
    delete rootItem;
}

QVariant CustomFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) {
            return tr("名称 (Name)");
        }
        // Add more headers if columnCount() > 1
    }
    return QVariant();
}

QModelIndex CustomFileModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem; // If parent is invalid, it means we want a child of the invisible root.
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem); // Create a QModelIndex for this child.
    return QModelIndex();
}

QModelIndex CustomFileModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    // If the parent is our invisible rootItem, then this item is a top-level item in the view.
    // Its parent in terms of QModelIndex should be an invalid QModelIndex().
    if (parentItem == rootItem || parentItem == nullptr) // parentItem == nullptr should ideally not happen if structured correctly
        return QModelIndex();

    // Otherwise, create a QModelIndex for the parent.
    // The row of the parent is its position in its own parent's child list.
    return createIndex(parentItem->row(), 0, parentItem);
}

int CustomFileModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0) // Only column 0 can have children in a tree model.
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int CustomFileModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1; // We have one column: Name (with checkbox)
}

QVariant CustomFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    if (role == Qt::DisplayRole && index.column() == 0) {
        return item->name();
    }

    // For Qt::CheckStateRole, now applies to both files and folders
    if (role == Qt::CheckStateRole && index.column() == 0) {
        return static_cast<int>(item->checkState());
    }

    if (role == Qt::ToolTipRole && index.column() == 0) {
        return item->path(); // Show full path as tooltip
    }

    return QVariant();
}

Qt::ItemFlags CustomFileModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (item->type() == TreeItem::File) {
        // Files are checkable, enabled, and selectable.
        return defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    } else if (item->type() == TreeItem::Folder) {
        // Folders are now checkable (tristate)
        return defaultFlags | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsUserTristate;
    }
    return defaultFlags;
}

bool CustomFileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() != 0)
        return false;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    if (role == Qt::CheckStateRole) {
        Qt::CheckState newState = static_cast<Qt::CheckState>(value.toInt());
        qDebug() << "setData for CheckStateRole: item" << item->name()
                 << "type:" << (item->type() == TreeItem::File ? "File" : "Folder")
                 << "current state:" << item->checkState()
                 << "new state:" << newState;

        if (item->checkState() == newState && item->type() == TreeItem::File) {
             qDebug() << "setData: State NOT changed for file" << item->name() << "(already" << newState << ").";
             return true; // For files, if state is same, do nothing more.
        }
        // For folders, even if newState is same as current (e.g. user clicks already checked folder),
        // we might need to propagate to children if their states are inconsistent.

        if (item->type() == TreeItem::File) {
            item->setCheckState(newState);
            emit dataChanged(index, index, {Qt::CheckStateRole});
            updateFolderCheckState(parent(index)); // Update parent folder state
            return true;
        } else if (item->type() == TreeItem::Folder) {
            // User clicked a folder checkbox.
            // The newState is what the view wants to set (Checked, Unchecked, or even PartiallyChecked if user could cycle through it)
            // Typically, a direct click on a folder checkbox toggles between Checked and Unchecked.
            // If it was PartiallyChecked, it usually becomes Checked.

            Qt::CheckState targetChildState;
            if (newState == Qt::PartiallyChecked) {
                // If user somehow forces folder to PartiallyChecked (e.g. not typical UI),
                // we interpret this as checking all children. Or it could be a no-op.
                // For simplicity, let's assume user interaction leads to Checked or Unchecked.
                // If the folder was partially checked, a click usually makes it fully checked.
                targetChildState = Qt::Checked;
                newState = Qt::Checked; // Folder itself also becomes checked
            } else {
                targetChildState = newState; // Should be Qt::Checked or Qt::Unchecked
            }

            // Set the folder's state first, then propagate.
            // The folder's state might be re-evaluated by updateFolderCheckState later if needed,
            // but direct user action on a folder checkbox implies a clear intent.
            if (item->checkState() != newState) {
                item->setCheckState(newState);
                emit dataChanged(index, index, {Qt::CheckStateRole});
            }

            propagateFolderStateToChildren(item, targetChildState, index);

            // After propagation, the folder's own state should align with what was set (Checked or Unchecked)
            // because all children were forced to that state.
            // If `newState` was `Qt::PartiallyChecked` and we made it `Qt::Checked`, this ensures consistency.
            if (item->checkState() != targetChildState) {
                 item->setCheckState(targetChildState); // Ensure it's not left partially checked from this operation
                 emit dataChanged(index, index, {Qt::CheckStateRole}); // Emit again if it changed from initial set
            }

            updateFolderCheckState(parent(index)); // Update parent of this folder
            return true;
        }
    }
    return false;
}

// Helper function for setAllCheckStates
void setAllCheckStatesRecursiveInternal(TreeItem* item, Qt::CheckState state) {
    if (!item) return;
    item->setCheckState(state);
    for (int i = 0; i < item->childCount(); ++i) {
        setAllCheckStatesRecursiveInternal(item->child(i), state);
    }
}

void CustomFileModel::setupModelData(const QString &currentPath, TreeItem *parent)
{
    QDir dir(currentPath);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist:" << currentPath;
        return;
    }

    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    dir.setSorting(QDir::Name | QDir::DirsFirst); // Sort by name, folders first

    QFileInfoList entries = dir.entryInfoList();

    for (const QFileInfo &entryInfo : entries) {
        if (entryInfo.isDir()) {
            TreeItem *folderItem = new TreeItem(entryInfo.fileName(), TreeItem::Folder, parent);
            folderItem->setPath(entryInfo.filePath());
            parent->appendChild(folderItem);
            setupModelData(entryInfo.filePath(), folderItem); // Recurse into subdirectory
        } else if (entryInfo.isFile()) {
            // Apply name filters if they are set
            bool passesFilter = nameFilters.isEmpty(); // If no filters, pass all files
            /* // Commenting out QRegExp based filtering as it's obsolete and not fully implemented
            if (!nameFilters.isEmpty()) {
                for (const QString &filter : nameFilters) {
                    // QRegExp rx(filter, Qt::CaseSensitive, QRegExp::Wildcard); // QRegExp is obsolete
                    // if (rx.exactMatch(entryInfo.fileName())) {
                    //     passesFilter = true;
                    //     break;
                    // }
                }
            }
            */

            if (passesFilter) {
                TreeItem *fileItem = new TreeItem(entryInfo.fileName(), TreeItem::File, parent);
                fileItem->setPath(entryInfo.filePath());
                parent->appendChild(fileItem);
            }
        }
    }
}

void CustomFileModel::toggleCheckState(const QModelIndex &index) {
    if (!index.isValid() || index.column() != 0) return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    Qt::CheckState currentState = item->checkState();
    Qt::CheckState newState;

    if (item->type() == TreeItem::File) {
        newState = (currentState == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
        // setData will emit dataChanged and update parent folder state
        setData(index, newState, Qt::CheckStateRole); 
    } else if (item->type() == TreeItem::Folder) {
        // When a folder is toggled:
        // If it's Unchecked or PartiallyChecked, it becomes Checked.
        // If it's Checked, it becomes Unchecked.
        if (currentState == Qt::Checked) {
            newState = Qt::Unchecked;
        } else { // Unchecked or PartiallyChecked
            newState = Qt::Checked;
        }
        // Use setData to ensure propagation and signals are handled correctly.
        // setData for a folder will propagate the newState to children.
        setData(index, newState, Qt::CheckStateRole);
    }
}

void CustomFileModel::setAllCheckStates(Qt::CheckState state) {
    beginResetModel();
    for (int i = 0; i < rootItem->childCount(); ++i) {
        setAllCheckStatesRecursiveInternal(rootItem->child(i), state);
    }
    // After setting all children, folder states are implicitly defined by their children.
    // No need to call updateFolderCheckState explicitly here as begin/endResetModel forces view refresh.
    endResetModel();
}


// This recursive part is now integrated into setAllCheckStates logic
// This specific version of setAllCheckStatesRecursive is no longer needed
// due to the new setAllCheckStatesRecursiveInternal and beginResetModel/endResetModel strategy.
// void CustomFileModel::setAllCheckStatesRecursive(TreeItem *item, Qt::CheckState state) {
//     for (int i = 0; i < item->childCount(); ++i) {
//         TreeItem *child = item->child(i);
//         if (child->type() == TreeItem::File) { // Should apply to folders too if we want them to also be checked/unchecked
//             if (child->checkState() != state) {
//                  child->setCheckState(state);
//                  // The dataChanged signal needs to be emitted by the caller (setAllCheckStates)
//                  // with the correct QModelIndex.
//             }
//         } // else if (child->type() == TreeItem::Folder) { // If folders also get the state
//           //  child->setCheckState(state);
//         // }
//         if (child->childCount() > 0) {
//             setAllCheckStatesRecursive(child, state);
//         }
//     }
// }

// The old setAllCheckStatesRecursive can be removed or commented out.
// For now, I will comment it out. If it's referenced elsewhere, those references would need updating.
// It seems `setAllCheckStates` is the main entry point and now uses `setAllCheckStatesRecursiveInternal`.


QStringList CustomFileModel::getCheckedFilesPaths() const {
    QStringList paths;
    getCheckedFilesRecursive(rootItem, paths);
    return paths;
}

void CustomFileModel::getCheckedFilesRecursive(TreeItem *item, QStringList &paths) const {
    for (int i = 0; i < item->childCount(); ++i) {
        TreeItem *child = item->child(i);
        if (child->type() == TreeItem::File && child->checkState() == Qt::Checked) {
            paths.append(child->path());
        }
        if (child->childCount() > 0) { // Recurse for folders
            getCheckedFilesRecursive(child, paths);
        }
    }
}

bool CustomFileModel::hasFiles() const {
    return hasFilesRecursive(rootItem);
}

bool CustomFileModel::hasFilesRecursive(TreeItem* item) const {
    for (int i = 0; i < item->childCount(); ++i) {
        TreeItem *child = item->child(i);
        if (child->type() == TreeItem::File) {
            return true; // Found at least one file
        }
        if (child->type() == TreeItem::Folder) {
            if (hasFilesRecursive(child)) { // Check subfolders
                return true;
            }
        }
    }
    return false;
}

void CustomFileModel::updateFolderCheckState(const QModelIndex &folderIndex)
{
    if (!folderIndex.isValid()) return;

    TreeItem *folderItem = static_cast<TreeItem*>(folderIndex.internalPointer());
    if (!folderItem || folderItem->type() != TreeItem::Folder) {
        return;
    }

    int childrenCheckedCount = 0;
    int childrenUncheckedCount = 0;
    int childrenPartialCount = 0;
    int relevantChildCount = 0; // Count of files and sub-folders

    for (int i = 0; i < folderItem->childCount(); ++i) {
        TreeItem *childItem = folderItem->child(i);
        if (!childItem) continue;

        relevantChildCount++; // All children (files or folders) are relevant for parent state

        if (childItem->type() == TreeItem::File) {
            if (childItem->checkState() == Qt::Checked) {
                childrenCheckedCount++;
            } else { // Qt::Unchecked (files are not tristate)
                childrenUncheckedCount++;
            }
        } else if (childItem->type() == TreeItem::Folder) {
            Qt::CheckState childFolderState = childItem->checkState();
            if (childFolderState == Qt::Checked) {
                childrenCheckedCount++;
            } else if (childFolderState == Qt::PartiallyChecked) {
                childrenPartialCount++;
            } else { // Qt::Unchecked
                childrenUncheckedCount++;
            }
        }
    }

    Qt::CheckState newState;
    if (relevantChildCount == 0) {
        newState = Qt::Unchecked; // Or Qt::Checked, depending on desired default for empty folders
    } else if (childrenPartialCount > 0) {
        newState = Qt::PartiallyChecked;
    } else if (childrenCheckedCount > 0 && childrenUncheckedCount > 0) {
        newState = Qt::PartiallyChecked;
    } else if (childrenCheckedCount == relevantChildCount) {
        newState = Qt::Checked;
    } else if (childrenUncheckedCount == relevantChildCount) {
        newState = Qt::Unchecked;
    } else {
        // This case should ideally not be reached if logic is correct
        // For example, if some children are checked and some are partial, it's partial.
        // If all are checked, it's checked. If all are unchecked, it's unchecked.
        // If a mix of checked and unchecked, it's partial.
        // The conditions above should cover these.
        // Defaulting to PartiallyChecked as a fallback if something is missed.
        newState = Qt::PartiallyChecked;
         qWarning() << "updateFolderCheckState: Unhandled state combination for folder" << folderItem->name()
                   << "Checked:" << childrenCheckedCount
                   << "Unchecked:" << childrenUncheckedCount
                   << "Partial:" << childrenPartialCount
                   << "TotalRelevant:" << relevantChildCount;
    }

    if (folderItem->checkState() != newState) {
        folderItem->setCheckState(newState);
        emit dataChanged(folderIndex, folderIndex, {Qt::CheckStateRole});

        QModelIndex parentModelIndex = parent(folderIndex);
        if (parentModelIndex.isValid()) {
            updateFolderCheckState(parentModelIndex);
        }
    }
}

void CustomFileModel::propagateFolderStateToChildren(TreeItem *folderItem, Qt::CheckState state, const QModelIndex &parentFolderIndex)
{
    if (!folderItem) return;

    // State to propagate to children is either Checked or Unchecked.
    // Folders themselves can become PartiallyChecked, but when a user checks/unchecks a folder,
    // its children are all checked or all unchecked.
    Qt::CheckState childStateToSet = (state == Qt::PartiallyChecked) ? Qt::Checked : state; // Default to checked if parent becomes partial? No, should be full check/uncheck.
    childStateToSet = (state == Qt::Checked) ? Qt::Checked : Qt::Unchecked;


    for (int i = 0; i < folderItem->childCount(); ++i) {
        TreeItem* childItem = folderItem->child(i);
        if (!childItem) continue;

        QModelIndex childIndex = index(i, 0, parentFolderIndex);
        if (!childIndex.isValid()) {
            qWarning() << "propagateFolderStateToChildren: Could not get valid QModelIndex for child" << childItem->name();
            continue;
        }

        if (childItem->checkState() != childStateToSet) {
            childItem->setCheckState(childStateToSet);
            emit dataChanged(childIndex, childIndex, {Qt::CheckStateRole});
        }

        if (childItem->type() == TreeItem::Folder) {
            // Recursive call to propagate to grandchildren
            propagateFolderStateToChildren(childItem, childStateToSet, childIndex);
        }
    }
}

void CustomFileModel::selectFilesByExtension(const QModelIndex &folderIndex, const QString &extension)
{
    TreeItem *folderItem;
    if (!folderIndex.isValid()) { // If folderIndex is invalid, operate on rootItem's children
        folderItem = rootItem;
        // qDebug() << "selectFilesByExtension: Operating on root level items.";
    } else {
        folderItem = static_cast<TreeItem*>(folderIndex.internalPointer());
        if (!folderItem || folderItem->type() != TreeItem::Folder) {
            qWarning() << "selectFilesByExtension: Index does not point to a valid folder item or root.";
            return;
        }
    }

    if (extension.isEmpty()) {
        qWarning() << "selectFilesByExtension: Extension string is empty.";
        return;
    }

    QString actualExtension = extension;
    if (!actualExtension.startsWith(".")) {
        actualExtension.prepend(".");
    }

    qDebug() << "selectFilesByExtension: Target Folder -" << (folderIndex.isValid() ? folderItem->name() : "Root") << "Extension -" << actualExtension;

    for (int i = 0; i < folderItem->childCount(); ++i) {
        TreeItem *childItem = folderItem->child(i);
        if (childItem && childItem->type() == TreeItem::File) {
            if (childItem->name().endsWith(actualExtension, Qt::CaseInsensitive)) {
                if (childItem->checkState() != Qt::Checked) {
                    QModelIndex childIndex;
                    if (!folderIndex.isValid()){ 
                        childIndex = index(i, 0, QModelIndex());
                    } else { 
                        childIndex = index(i, 0, folderIndex);
                    }

                    if (childIndex.isValid()) {
                        setData(childIndex, Qt::Checked, Qt::CheckStateRole); 
                    } else {
                        qWarning() << "Could not get valid QModelIndex for child" << childItem->name() << "in selectFilesByExtension";
                    }
                }
            }
        }
    }
    // Rely on setData's existing update propagation.
}

// New methods for recursive selection by extension
void CustomFileModel::selectFilesByExtensionRecursive(const QModelIndex& startIndex, const QString &extension)
{
    if (extension.isEmpty()) {
        qWarning() << "selectFilesByExtensionRecursive: Extension string is empty.";
        return;
    }

    // Normalize extension string (ensure it starts with a dot)
    QString normalizedExtension = extension;
    if (!normalizedExtension.startsWith(".")) {
        normalizedExtension.prepend(".");
    }

    qDebug() << "Starting recursive selection. StartIndex valid:" << startIndex.isValid() << "Extension:" << normalizedExtension;

    // We will be changing data, potentially a lot. It's good practice to emit signals
    // that can tell the view to prepare for a larger update, though for CheckStateRole,
    // individual dataChanged signals are often handled efficiently by Qt views.
    // For simplicity and to leverage existing updateFolderCheckState propagation,
    // we'll rely on individual setData calls emitting dataChanged.

    selectFilesByExtensionRecursiveHelper(startIndex, normalizedExtension);

    // After the recursive process, if the starting point was a valid folder,
    // its check state might need to be updated based on its children's new states.
    // However, the `setData` calls on files will trigger `updateFolderCheckState` for their parents,
    // which will propagate upwards. So, an explicit call here for `startIndex` might be redundant
    // if `startIndex` is a parent of modified files, or it might be needed if `startIndex` itself
    // didn't have direct file children changed but its subfolders did.
    // To be safe, and ensure the top-most affected folder in this operation (startIndex, if valid)
    // gets its state correctly evaluated *after* all its descendants are processed:
    if (startIndex.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(startIndex.internalPointer());
        if (item && item->type() == TreeItem::Folder) {
             // This ensures that if a sub-sub-folder caused a change that partially checks `startIndex`,
             // `startIndex` reflects this after the whole operation.
            updateFolderCheckState(startIndex);
        }
    }
    // If startIndex is invalid (QModelIndex()), it means we started from the rootItem's children.
    // The update propagation should have handled all visible top-level folders.
}


void CustomFileModel::populateModelFromFileList(const QStringList &absoluteFilePaths)
{
    beginResetModel(); // Signal that the model is about to change drastically

    // Clear existing model data
    if (rootItem) {
        rootItem->clearChildren(); // Assumes TreeItem::clearChildren() correctly deletes child TreeItems
    } else {
        // This case should ideally not be hit if the model is always constructed
        // with a rootPath or if a default rootItem is created in constructor.
        // Let's ensure rootItem is valid.
        rootItem = new TreeItem(QStringLiteral("__InvisibleRoot__"), TreeItem::Folder, nullptr);
    }

    // Populate with new file paths
    for (const QString &filePath : absoluteFilePaths) {
        QFileInfo fileInfo(filePath);
        // We only add files, not directories, directly from the list.
        // And only if they exist.
        if (filePath.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
            qWarning() << "CustomFileModel::populateModelFromFileList: Skipping invalid, non-existent, or non-file path:" << filePath;
            continue;
        }

        // Create TreeItem for the file. It will be a child of the invisible rootItem.
        // These files will appear at the root level of the tree view.
        TreeItem *fileItem = new TreeItem(fileInfo.fileName(), TreeItem::File, rootItem);
        fileItem->setPath(fileInfo.absoluteFilePath()); // Store the full absolute path
        fileItem->setCheckState(Qt::Checked);       // Mark as checked by default
        rootItem->appendChild(fileItem);
    }

    endResetModel(); // Signal that the model has been reset
}


void CustomFileModel::selectFilesByExtensionRecursiveHelper(const QModelIndex& currentIndex, const QString &normalizedExtension)
{
    TreeItem *parentItem;
    if (!currentIndex.isValid()) { // Operating on children of the invisible rootItem
        parentItem = rootItem;
    } else {
        parentItem = static_cast<TreeItem*>(currentIndex.internalPointer());
        // Ensure the current item itself is a folder before proceeding with its children
        if (!parentItem || parentItem->type() != TreeItem::Folder) {
            return;
        }
    }

    for (int i = 0; i < parentItem->childCount(); ++i) {
        QModelIndex childModelIndex = index(i, 0, currentIndex);
        if (!childModelIndex.isValid()) {
            continue;
        }

        TreeItem *childItem = static_cast<TreeItem*>(childModelIndex.internalPointer());
        if (!childItem) {
            continue;
        }

        if (childItem->type() == TreeItem::File) {
            if (childItem->name().endsWith(normalizedExtension, Qt::CaseInsensitive)) {
                if (childItem->checkState() != Qt::Checked) {
                    setData(childModelIndex, Qt::Checked, Qt::CheckStateRole);
                }
            }
        } else if (childItem->type() == TreeItem::Folder) {
            selectFilesByExtensionRecursiveHelper(childModelIndex, normalizedExtension); // Recurse into subfolder
        }
    }
}