// treeitem.h
// Helper class for CustomFileModel to represent items in the tree.

#ifndef TREEITEM_H
#define TREEITEM_H

#include <QList>
#include <QVariant>
#include <QString>
#include <QCoreApplication> // For tr // Retained if QCoreApplication::translate is used elsewhere, or for general Qt types

class TreeItem
{
    // Q_DECLARE_TR_FUNCTIONS(TreeItem) // REMOVED to reduce potential issues, tr() is not used in treeitem.cpp

public:
    enum ItemType { Folder, File };

    explicit TreeItem(const QString &name, ItemType type, TreeItem *parentItem = nullptr);
    ~TreeItem();

    void appendChild(TreeItem *child);

    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const; // For our model, it's 1 (name/checkbox)
    QVariant data(int column) const; // We'll use Qt::CheckStateRole for checkbox
    int row() const;
    TreeItem *parentItem();

    QString name() const;
    QString path() const; // Full path
    void setPath(const QString& path);

    ItemType type() const;

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);
    void clearChildren();


private:
    QString itemName;
    QString itemPath;
    ItemType itemType;
    Qt::CheckState itemCheckState;

    QList<TreeItem*> childItems;
    TreeItem *parentItm;
};

#endif // TREEITEM_H 