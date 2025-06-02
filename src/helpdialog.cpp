#include "helpdialog.h"
#include <QTextBrowser>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug> // For debugging

HelpDialog::HelpDialog(const QString &title, const QString &markdownFilePath, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setMinimumSize(600, 400); // Set a reasonable default size

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    textBrowser = new QTextBrowser(this);
    textBrowser->setReadOnly(true);
    // QTextBrowser supports basic Markdown. For more complex rendering,
    // one might need a dedicated Markdown library and render to HTML first.
    textBrowser->setOpenExternalLinks(true); // Open links in external browser

    mainLayout->addWidget(textBrowser);

    closeButton = new QPushButton(tr("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, &HelpDialog::accept);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Load and display the Markdown content
    QFile file(markdownFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setCodec("UTF-8"); // Ensure UTF-8 is used for reading
        QString markdownContent = in.readAll();
        textBrowser->setMarkdown(markdownContent); // QTextBrowser can render Markdown
        file.close();
    } else {
        qWarning() << "HelpDialog: Could not open markdown file:" << markdownFilePath << file.errorString();
        textBrowser->setPlainText(tr("Could not load help content from: %1\n\nError: %2").arg(markdownFilePath, file.errorString()));
        // QMessageBox::warning(this, tr("Error Loading Help"), tr("Could not load help content from: %1").arg(markdownFilePath));
        // Displaying the error in the textBrowser itself is often better for a non-blocking info.
        // If a modal QMessageBox is truly desired here, it can be uncommented.
    }
}

HelpDialog::~HelpDialog()
{
    // Qt's parent-child ownership will handle deletion of child widgets
}
