// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QLineEdit>       // Added for QLineEdit
#include <QPushButton>     // Added for QPushButton
#include <QTreeView>       // Added for QTreeView
#include <QStatusBar>      // Added for QStatusBar
#include <QProgressBar>    // Added for QProgressBar
#include <QMenu>           // Added for QMenu
#include <QAction>         // Added for QAction

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class CustomFileModel;
class FileMergerLogic;
class QTreeView;
class QLineEdit;
class QPushButton;
class QStatusBar;
class QGroupBox; // Added missing include for QGroupBox
class QLabel;    // Added missing include for QLabel
class QVBoxLayout; // Added missing include for QVBoxLayout
class QHBoxLayout; // Added missing include for QHBoxLayout
class QProgressBar; // Added missing include for QProgressBar


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void browseFolder();
    void onTreeViewClicked(const QModelIndex &index);
    void selectAllFiles();
    void deselectAllFiles();
    void startMerge();
    void updateStatus(const QString &message);
    void mergeProcessFinished(bool success, const QString &messageOrPath);
    void updateProgressBar(int value);
    void showContextMenu(const QPoint &point);
    void handleSelectByExtensionTriggered(const QModelIndex& folderIndex, const QString& extension);
    void onRecursiveSelectByExtensionTriggered();

private:
    // void setupUi(); // Helper to set up UI elements if not using .ui file
    void connectSignalsAndSlots();

    // UI Elements (can be defined in a .ui file and accessed via ui->elementName)
    QLineEdit *folderPathLineEdit;
    QPushButton *browseButton;
    QTreeView *fileTreeView;
    QPushButton *selectAllButton;
    QPushButton *deselectAllButton;
    QPushButton *mergeButton;
    QStatusBar *statusBar; // Will use QMainWindow's default status bar
    QProgressBar *progressBar; // Added for the progress bar

    QAction *actionRecursiveSelectByExtension; // Action for new recursive selection

    CustomFileModel *fileModel;
    FileMergerLogic *mergerLogic;
    QString currentFolderPath;
};
#endif // MAINWINDOW_H 