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

    if (role == Qt::CheckStateRole && index.column() == 0 && item->type() == TreeItem::File) {
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
        // Folders are enabled and selectable, but not checkable directly in this design.
        return defaultFlags | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    return defaultFlags;
}

bool CustomFileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() != 0)
        return false;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    if (role == Qt::CheckStateRole && item->type() == TreeItem::File) {
        Qt::CheckState newState = static_cast<Qt::CheckState>(value.toInt());
        qDebug() << "setData for CheckStateRole: item" << item->name()
                 << "current state:" << item->checkState()
                 << "new state:" << newState;

        if (item->checkState() != newState) {
            item->setCheckState(newState); // This calls TreeItem::setCheckState
            qDebug() << "setData: State changed for" << item->name() << "to" << newState << ". Emitting dataChanged.";
            emit dataChanged(index, index, {Qt::CheckStateRole});
            return true;
        } else {
            qDebug() << "setData: State NOT changed for" << item->name() << "(already" << newState << "). View trying to set same state.";
            return true; // Return true even if state is not changed
        }
    }
    return false;
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
    // Using beginResetModel() / endResetModel() is a blunt instrument.
    // It works but causes the view to lose its current expansion state and selection.
    // A more refined approach is to iterate and emit dataChanged for each affected index.

    QVector<QModelIndex> indicesToUpdate; // Use QVector for QModelIndex

    // We need a way to get QModelIndex for each TreeItem.
    // This recursive function will find all file items and if their state changes,
    // it will create the QModelIndex for them and add to list.
    std::function<void(TreeItem*, const QModelIndex&)> collectIndicesRecursively =
        [&](TreeItem* currentItem, const QModelIndex& parentIndex) {
        for (int i = 0; i < currentItem->childCount(); ++i) {
            TreeItem* child = currentItem->child(i);
            QModelIndex childIndex = index(i, 0, parentIndex); // Get the model index for the child

            if (child->type() == TreeItem::File) {
                if (child->checkState() != state) {
                    // No need to set it here, setData will be called by the view or manually
                    // Actually, we should set it here, then emit dataChanged.
                    child->setCheckState(state);
                    indicesToUpdate.append(childIndex);
                }
            }
            if (child->childCount() > 0) {
                collectIndicesRecursively(child, childIndex);
            }
        }
    };

    // Start recursion from the children of the invisible root item
    if (rootItem->childCount() > 0) {
        collectIndicesRecursively(rootItem, QModelIndex()); // QModelIndex() for parent of top-level items
    }

    // Emit dataChanged for all collected indices
    // This is still potentially many signals. For very large models,
    // emitting dataChanged for entire ranges (parent's first to last child)
    // might be better if many items under one parent change.
    // However, individual changes are fine for moderate numbers.

    // Qt's views are optimized to handle dataChanged signals.
    // For a "select all" / "deselect all", it might be a lot of signals.
    // If performance becomes an issue here for extremely large visible lists,
    // one might consider emitting dataChanged for the parent QModelIndex of a block of changed items,
    // covering the range from the first changed child row to the last changed child row.
    // However, for simplicity and correctness, emitting for each individual changed index is robust.

    if (!indicesToUpdate.isEmpty()) {
        // One way to optimize signals slightly (if items are contiguous and under same parent):
        // Group indices by parent and emit dataChanged for ranges.
        // For now, individual signals:
        for (const QModelIndex& idx : indicesToUpdate) {
            emit dataChanged(idx, idx, {Qt::CheckStateRole});
        }
        // A simpler alternative that works but might be less "optimal" for the view than specific ranges:
        // beginResetModel();
        // setAllCheckStatesRecursive(rootItem, state); // This function would just set the state
        // endResetModel();
    }
}


// This recursive part is now integrated into setAllCheckStates logic
void CustomFileModel::setAllCheckStatesRecursive(TreeItem *item, Qt::CheckState state) {
    for (int i = 0; i < item->childCount(); ++i) {
        TreeItem *child = item->child(i);
        if (child->type() == TreeItem::File) {
            if (child->checkState() != state) {
                 child->setCheckState(state);
                 // The dataChanged signal needs to be emitted by the caller (setAllCheckStates)
                 // with the correct QModelIndex.
            }
        }
        if (child->childCount() > 0) {
            setAllCheckStatesRecursive(child, state);
        }
    }
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