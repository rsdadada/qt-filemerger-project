#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

class QTextBrowser; // Forward declaration
class QPushButton;    // Forward declaration

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(const QString &title, const QString &markdownFilePath, QWidget *parent = nullptr);
    ~HelpDialog();

private:
    QTextBrowser *textBrowser;
    QPushButton *closeButton;
};

#endif // HELPDIALOG_H
