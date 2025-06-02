// treeitem.cpp

#include "treeitem.h"
#include <QtAlgorithms> // For qDeleteAll
#include <QtGlobal> // For qWarning, Q_ASSERT

TreeItem::TreeItem(const QString &name, ItemType type, TreeItem *parent)
    : itemName(name), itemPath(), itemType(type), itemCheckState(Qt::Unchecked), parentItm(parent)
{
    // itemPath can be set later using setPath()
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems); // Uses childItems (no m_ prefix)
}

void TreeItem::appendChild(TreeItem *item)
{
    if (item) { 
        // item->parentItm = this; // Already done by constructor if parent is passed, or should be done by caller logic
        childItems.append(item); // Uses childItems
    }
}

TreeItem *TreeItem::child(int row)
{
    if (row < 0 || row >= childItems.size()) // Uses childItems
        return nullptr;
    return childItems.at(row); // Uses childItems
}

int TreeItem::childCount() const
{
    return childItems.count(); // Uses childItems
}

int TreeItem::columnCount() const
{
    return 1; // As per user's .h comment
}

QVariant TreeItem::data(int column) const
{
    // Matches logic in user's .cpp for data(int) related to itemName
    if (column == 0) {
        return itemName; // Uses itemName
    }
    // Note: Original user .cpp also had comments about CheckStateRole being handled by model.
    // This version aligns with the .h which implies this class can provide display data.
    return QVariant();
}

TreeItem *TreeItem::parentItem()
{
    return parentItm; // Uses parentItm
}

int TreeItem::row() const
{
    if (parentItm) // Uses parentItm
        return parentItm->childItems.indexOf(const_cast<TreeItem*>(this)); // Uses childItems
    return 0; 
}

QString TreeItem::name() const {
    return itemName; // Uses itemName
}

QString TreeItem::path() const {
    return itemPath; // Uses itemPath
}

void TreeItem::setPath(const QString& path) {
    itemPath = path; // Uses itemPath
}

TreeItem::ItemType TreeItem::type() const {
    return itemType; // Uses itemType
}

Qt::CheckState TreeItem::checkState() const {
    return itemCheckState; // Uses itemCheckState
}

void TreeItem::setCheckState(Qt::CheckState state) {
    itemCheckState = state; // Uses itemCheckState
}

void TreeItem::clearChildren()
{
    qDeleteAll(childItems); // Uses childItems
    childItems.clear();     // Uses childItems
}