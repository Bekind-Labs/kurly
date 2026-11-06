#pragma once

#include <QMainWindow>
#include <QTranslator>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QToolButton;
class QFormLayout;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void updateCommand();
    void copyCommand();
    void addHeaderRow(const QString &name = {}, const QString &value = {});
    void removeSelectedHeaders();
    void setLanguage(const QString &code);

private:
    void buildUi();
    void retranslateUi();

    QTranslator m_translator;

    // Request
    QLabel *m_urlLabel, *m_methodLabel;
    QLineEdit *m_url;
    QComboBox *m_method;
    QGroupBox *m_headersBox, *m_bodyBox, *m_authBox, *m_optionsBox;
    QTableWidget *m_headers;
    QPushButton *m_addHeader, *m_removeHeader;
    QPlainTextEdit *m_body;
    // Auth / misc
    QLabel *m_userLabel, *m_agentLabel, *m_timeoutLabel, *m_outputLabel;
    QLineEdit *m_user, *m_agent, *m_output;
    QSpinBox *m_timeout;
    // Flags
    QCheckBox *m_follow, *m_insecure, *m_verbose, *m_silent, *m_include,
        *m_fail, *m_compressed;
    // Output formatting
    QCheckBox *m_longOpts, *m_multiline;
    // Right pane
    QLabel *m_resultLabel, *m_languageLabel;
    QPlainTextEdit *m_result;
    QPushButton *m_copy;
    QComboBox *m_language;
};
