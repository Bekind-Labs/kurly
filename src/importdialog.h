#pragma once

#include "curlparser.h"

#include <QDialog>

class QLabel;
class QPlainTextEdit;

// Lets the user paste an existing curl command (e.g. Chrome DevTools'
// "Copy as cURL") to load into the form.
class ImportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImportDialog(QWidget *parent = nullptr);

    const CurlParser::Result &result() const { return m_result; }

    void accept() override;

private:
    QPlainTextEdit *m_input;
    QLabel *m_error;
    CurlParser::Result m_result;
};
