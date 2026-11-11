#pragma once

#include <QHash>
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
class QTimer;
class QScrollArea;
struct CurlOptions;
class LibraryPanel;
class RequestRunner;
class RequestStore;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    // libraryPath: where the request library is kept; empty for the default
    // location (RequestStore::defaultPath()).
    explicit MainWindow(QWidget *parent = nullptr, const QString &libraryPath = QString());
    ~MainWindow() override;

    RequestStore *store() const { return m_store; }
    LibraryPanel *library() const { return m_library; }

    // The request currently described by the form, and loading one into it.
    CurlOptions currentOptions() const;
    void loadOptions(const CurlOptions &o);

    // The curl executable used by Run. Defaults to "curl" on the PATH.
    void setCurlProgram(const QString &program);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void updateCommand();
    void copyCommand();
    void addHeaderRow(const QString &name = {}, const QString &value = {});
    void removeSelectedHeaders();
    void setLanguage(const QString &code);
    void runOrStop();
    void renderResponse();
    void importCommand();
    void showRequest(const QString &id);
    void setLibraryVisible(bool visible);
    void newRequest();

private:
    void buildUi();
    void retranslateUi();
    void updateRunState();
    // The runner for a request; created on first use when `create` is set.
    RequestRunner *runnerFor(const QString &id, bool create = false);
    RequestRunner *currentRunner() const { return m_runners.value(m_shownId); }
    void pruneRunners();
    void reportLibraryProblem(int loadResult);
    QString exitCodeHint(int code) const;

    // The request library, and one runner (with its response) per request.
    RequestStore *m_store = nullptr;
    LibraryPanel *m_library = nullptr;
    QPushButton *m_libraryToggle = nullptr;
    QHash<QString, RequestRunner *> m_runners;
    QString m_shownId;      // the request shown in the form
    bool m_loading = false; // filling the form from the library: don't write back
    bool m_saveFailureShown = false;
    QTimer *m_renderTimer = nullptr;
    QString m_curlProgram = QStringLiteral("curl");

    QTranslator m_translator;   // Kurly's own strings
    QTranslator m_qtTranslator; // Qt's built-in strings (OK buttons, context menus)

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
