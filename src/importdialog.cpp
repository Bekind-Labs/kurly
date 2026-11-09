#include "importdialog.h"

#include "spacing.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ImportDialog::ImportDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Import curl Command"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(Spacing::large, Spacing::large, Spacing::large, Spacing::large);
    layout->setSpacing(Spacing::small);

    auto *intro = new QLabel(
        tr("Paste a curl command, for example from your browser's developer tools "
           "(Network tab → right-click a request → Copy → Copy as cURL). "
           "Both bash and Windows cmd formats are supported."));
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_input = new QPlainTextEdit;
    m_input->setObjectName(QStringLiteral("input"));
    m_input->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_input->setPlaceholderText(QStringLiteral("curl 'https://example.com/api' -H 'accept: application/json'"));
    m_input->setTabChangesFocus(true);
    layout->addWidget(m_input, 1);

    m_error = new QLabel;
    m_error->setObjectName(QStringLiteral("error"));
    m_error->setWordWrap(true);
    m_error->setStyleSheet(QStringLiteral("color: #c62828;"));
    m_error->hide();
    layout->addWidget(m_error);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Import"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    layout->addSpacing(Spacing::large - Spacing::small); // buttons are a separate group
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &ImportDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ImportDialog::reject);
    connect(m_input, &QPlainTextEdit::textChanged, m_error, &QLabel::hide);

    // Save a step when a curl command is already on the clipboard.
    const QString clip = QApplication::clipboard()->text().trimmed();
    if (CurlParser::parse(clip).error != CurlParser::Error::NotCurl && !clip.isEmpty()) {
        m_input->setPlainText(clip);
        m_input->selectAll();
    }

    resize(36 * Spacing::gridUnit, 22 * Spacing::gridUnit);
}

void ImportDialog::accept()
{
    m_result = CurlParser::parse(m_input->toPlainText());
    QString message;
    switch (m_result.error) {
    case CurlParser::Error::None:
        QDialog::accept();
        return;
    case CurlParser::Error::Empty:
        message = tr("Paste a curl command first.");
        break;
    case CurlParser::Error::UnterminatedQuote:
        message = tr("A quote is never closed. Make sure you copied the whole command.");
        break;
    case CurlParser::Error::NotCurl:
        message = tr("This doesn't look like a curl command. It should start with \"curl\".");
        break;
    case CurlParser::Error::NoUrl:
        message = tr("No URL was found in the command.");
        break;
    }
    m_error->setText(message);
    m_error->show();
    m_input->setFocus();
}
