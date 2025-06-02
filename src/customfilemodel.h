// customfilemodel.h

#ifndef CUSTOMFILEMODEL_H
#define CUSTOMFILEMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QDir>
#include <QCoreApplication> // For tr

class TreeItem; // Forward declaration

class CustomFileModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit CustomFileModel(const QString &rootPath, QObject *parent = nullptr);
    ~CustomFileModel();

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // Custom methods
    void toggleCheckState(const QModelIndex &index);
    void setAllCheckStates(Qt::CheckState state);
    QStringList getCheckedFilesPaths() const;
    bool hasFiles() const;
    void selectFilesByExtension(const QModelIndex &folderIndex, const QString &extension);
    void selectFilesByExtensionRecursive(const QModelIndex& startIndex, const QString &extension);
    void populateModelFromFileList(const QStringList &absoluteFilePaths);

private:
    void setupModelData(const QString &rootPath, TreeItem *parent);
    void getCheckedFilesRecursive(TreeItem *item, QStringList &paths) const;
    void setAllCheckStatesRecursive(TreeItem *item, Qt::CheckState state); // Removed changedIndices
    bool hasFilesRecursive(TreeItem* item) const;
    void updateFolderCheckState(const QModelIndex &folderIndex);
    void propagateFolderStateToChildren(TreeItem *folderItem, Qt::CheckState state, const QModelIndex &parentFolderIndex);
    void selectFilesByExtensionRecursiveHelper(const QModelIndex& currentIndex, const QString &normalizedExtension);


    TreeItem *rootItem;
    QStringList nameFilters; // e.g., "*.txt", "*.log" - currently allows all files/folders
};

#endif // CUSTOMFILEMODEL_H 