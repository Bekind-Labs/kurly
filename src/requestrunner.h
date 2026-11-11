#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringDecoder>
#include <QStringList>

// Runs curl for one request and collects its output. Each saved request gets
// its own runner, so switching requests keeps every response and a running
// request carries on in the background. GUI-independent.
class RequestRunner : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Idle,     // never run
        Running,
        Finished, // exited by itself (see exitCode())
        Crashed,  // killed: by stop() (stopRequested()) or otherwise
        NotFound, // curl couldn't be started
    };

    // Output is capped to keep memory and the UI responsive.
    static constexpr qsizetype kMaxChars = 2 * 1024 * 1024;

    explicit RequestRunner(QObject *parent = nullptr);
    ~RequestRunner() override; // kills a running curl

    void setProgram(const QString &program) { m_program = program; }
    // The full path of the curl the last run used, e.g. "/usr/bin/curl";
    // empty if it couldn't be found.
    QString resolvedProgram() const { return m_resolvedProgram; }

    // Starts curl with these arguments (as from CurlCommand::arguments()).
    // Returns false if they're empty or a run is already in progress.
    bool start(QStringList arguments);
    void stop();

    State state() const { return m_state; }
    bool isRunning() const { return m_state == State::Running; }
    int exitCode() const { return m_exitCode; }
    qint64 elapsedMs() const { return m_elapsedMs; }
    bool stopRequested() const { return m_stopRequested; }

    // Everything in arrival order, and stdout (the response) and stderr
    // (curl's messages) separately.
    const QString &output() const { return m_output; }
    const QString &standardOutput() const { return m_stdout; }
    const QString &standardError() const { return m_stderr; }
    bool isTruncated() const { return m_truncated; }

signals:
    void outputChanged();
    void stateChanged();

private:
    void read(QProcess::ProcessChannel channel);
    void finished(int exitCode, QProcess::ExitStatus status);
    void failedToStart(QProcess::ProcessError error);

    QString m_program = QStringLiteral("curl");
    QString m_resolvedProgram;
    QProcess *m_process = nullptr;
    State m_state = State::Idle;
    int m_exitCode = 0;
    qint64 m_elapsedMs = 0;
    bool m_stopRequested = false;
    QElapsedTimer m_elapsed;
    QString m_output, m_stdout, m_stderr;
    bool m_truncated = false;
    QStringDecoder m_outDecoder{QStringDecoder::Utf8};
    QStringDecoder m_errDecoder{QStringDecoder::Utf8};
};
