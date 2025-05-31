// treeitem.cpp

#include "treeitem.h"
#include <QtAlgorithms> // For qDeleteAll

TreeItem::TreeItem(const QString &name, ItemType type, TreeItem *parent)
    : itemName(name), itemType(type), itemCheckState(Qt::Unchecked), parentItm(parent)
{
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

void TreeItem::appendChild(TreeItem *item)
{
    if (item) { // Basic check
        item->parentItm = this; // Ensure parent is set correctly
        childItems.append(item);
    }
}

TreeItem *TreeItem::child(int row)
{
    if (row < 0 || row >= childItems.size())
        return nullptr;
    return childItems.at(row);
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::columnCount() const
{
    // We only display one column (name with checkbox)
    return 1;
}

QVariant TreeItem::data(int column) const
{
    // This method in TreeItem is mostly for returning the display name.
    // Other roles like CheckStateRole are handled directly by CustomFileModel
    // based on item's properties (like checkState() or type()).
    if (column == 0) {
        return itemName;
    }
    return QVariant();
}

TreeItem *TreeItem::parentItem()
{
    return parentItm;
}

int TreeItem::row() const
{
    if (parentItm)
        return parentItm->childItems.indexOf(const_cast<TreeItem*>(this));
    return 0; // Should not happen for items in the model other than the conceptual root's direct children
}

QString TreeItem::name() const {
    return itemName;
}

QString TreeItem::path() const {
    return itemPath;
}

void TreeItem::setPath(const QString& path) {
    itemPath = path;
}

TreeItem::ItemType TreeItem::type() const {
    return itemType;
}

Qt::CheckState TreeItem::checkState() const {
    // Folders are not checkable in this design, so they don't really have a check state.
    // However, if a folder could be "partially checked" if some children are,
    // that logic would go here or be managed by the model.
    // For now, only files have a meaningful check state.
    // This method now returns the itemCheckState directly, which can be
    // Unchecked, PartiallyChecked, or Checked for folders, as managed by the model.
    return itemCheckState;
}

void TreeItem::setCheckState(Qt::CheckState state) {
    // This method now sets the itemCheckState directly for both files and folders.
    // The CustomFileModel will be responsible for determining the appropriate
    // state for a folder (e.g., Qt::PartiallyChecked).
    itemCheckState = state;
}