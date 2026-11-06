#pragma once

#include <QElapsedTimer>
#include <QMainWindow>
#include <QProcess>
#include <QStringDecoder>
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
class QTimer;
class QScrollArea;
struct CurlOptions;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // The request currently described by the form, and loading one into it.
    CurlOptions currentOptions() const;
    void loadOptions(const CurlOptions &o);

    // The curl executable used by Run. Defaults to "curl" on the PATH.
    void setCurlProgram(const QString &program) { m_curlProgram = program; }

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void updateCommand();
    void copyCommand();
    void addHeaderRow(const QString &name = {}, const QString &value = {});
    void removeSelectedHeaders();
    void setLanguage(const QString &code);
    void runOrStop();
    void readStdout();
    void readStderr();
    void renderResponse();
    void processFinished(int exitCode, QProcess::ExitStatus status);
    void processError(QProcess::ProcessError error);
    void importCommand();

private:
    void buildUi();
    void retranslateUi();
    void appendOutput(const QString &text, bool isStdout);
    void updateRunState();
    QString exitCodeHint(int code) const;

    enum class RunState { Idle, Running, Finished, Crashed, NotFound };
    RunState m_runState = RunState::Idle;
    int m_exitCode = 0;
    qint64 m_elapsedMs = 0;
    // curl's output: everything in arrival order (for the raw view), plus
    // stdout (the response, for formatting) and stderr (curl's messages).
    QString m_rawOutput;
    QString m_stdout;
    QString m_stderr;
    bool m_truncated = false;
    QTimer *m_renderTimer = nullptr;
    bool m_stopRequested = false;
    QProcess *m_process = nullptr;
    QString m_curlProgram = QStringLiteral("curl");
    QElapsedTimer m_elapsed;
    QStringDecoder m_outDecoder{QStringDecoder::Utf8};
    QStringDecoder m_errDecoder{QStringDecoder::Utf8};

    QTranslator m_translator;

    // Request
    QLabel *m_urlLabel, *m_methodLabel;
    QLineEdit *m_url;
    QComboBox *m_method;
    QGroupBox *m_headersBox, *m_bodyBox, *m_authBox, *m_optionsBox;
    QTableWidget *m_headers;
    QPushButton *m_addHeader, *m_removeHeader;
    QPushButton *m_import = nullptr;
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
    QPushButton *m_copy = nullptr, *m_run = nullptr;
    QLabel *m_responseLabel = nullptr, *m_status = nullptr;
    QPlainTextEdit *m_response;
    QCheckBox *m_pretty = nullptr;
    QScrollArea *m_formScroll = nullptr;
    QComboBox *m_language;
};
