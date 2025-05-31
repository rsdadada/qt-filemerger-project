#include "customfilemodel.h"
#include "treeitem.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QApplication> // For style icons if used
#include <QStyle>       // For style icons if used
#include <functional>   // Required for std::function

CustomFileModel::CustomFileModel(const QString &rootPath, QObject *parent)
    : QAbstractItemModel(parent)
{
    // The rootItem in QAbstractItemModel is conceptual (QModelIndex()).
    // We create our own TreeItem that acts as the invisible root for our data.
    // The actual displayed root items will be children of this conceptual rootItem.
    rootItem = new TreeItem(tr("RootNode"), TreeItem::Folder); // Invisible root, name is not shown
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
        return item->checkState();
    }

    if (role == Qt::ToolTipRole && index.column() == 0) {
        return item->path(); // Show full path as tooltip
    }

    // Optional: Add icons for file/folder
    if (role == Qt::DecorationRole && index.column() == 0) {
        if (item->type() == TreeItem::Folder)
            return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        else if (item->type() == TreeItem::File)
            return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
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
        return defaultFlags | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsTristate;
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

    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
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
    if (item->type() == TreeItem::File) {
        Qt::CheckState currentState = item->checkState();
        Qt::CheckState newState = (currentState == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
        setData(index, newState, Qt::CheckStateRole); // This will emit dataChanged
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
}


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
    if (!folderIndex.isValid()) {
        qWarning() << "selectFilesByExtension: Invalid folderIndex.";
        return;
    }

    TreeItem *folderItem = static_cast<TreeItem*>(folderIndex.internalPointer());
    if (!folderItem || folderItem->type() != TreeItem::Folder) {
        qWarning() << "selectFilesByExtension: Index does not point to a valid folder item.";
        return;
    }

    if (extension.isEmpty()) {
        qWarning() << "selectFilesByExtension: Extension string is empty.";
        return;
    }

    QString actualExtension = extension;
    if (!actualExtension.startsWith(".")) {
        actualExtension.prepend(".");
    }

    qDebug() << "selectFilesByExtension: Folder -" << folderItem->name() << "Extension -" << actualExtension;

    for (int i = 0; i < folderItem->childCount(); ++i) {
        TreeItem *childItem = folderItem->child(i);
        if (childItem && childItem->type() == TreeItem::File) {
            if (childItem->name().endsWith(actualExtension, Qt::CaseInsensitive)) {
                if (childItem->checkState() != Qt::Checked) {
                    childItem->setCheckState(Qt::Checked);
                    QModelIndex childIndex = index(i, 0, folderIndex);
                    if (childIndex.isValid()) {
                        qDebug() << "Emitting dataChanged for" << childItem->name();
                        emit dataChanged(childIndex, childIndex, {Qt::CheckStateRole});
                    } else {
                        qWarning() << "Could not get valid QModelIndex for child" << childItem->name();
                    }
                }
            }
        }
    }
    // After iterating through all children, the folder's state might need to be updated.
    updateFolderCheckState(folderIndex);
}