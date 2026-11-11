#include "requestrunner.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

RequestRunner::RequestRunner(QObject *parent)
    : QObject(parent)
{
}

RequestRunner::~RequestRunner()
{
    // Don't report back into an object that's being destroyed.
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

bool RequestRunner::start(QStringList arguments)
{
    if (m_process || arguments.isEmpty())
        return false;
    // Without a terminal, curl prints a progress table to stderr, which would
    // clutter the output. This flag hides it but still shows errors.
    arguments.prepend(QStringLiteral("--no-progress-meter"));

    m_output.clear();
    m_stdout.clear();
    m_stderr.clear();
    m_truncated = false;
    m_stopRequested = false;
    m_exitCode = 0;
    m_elapsedMs = 0;
    m_outDecoder.resetState();
    m_errDecoder.resetState();

    // Record which curl this is, for error reports ("Ran /usr/bin/curl").
    m_resolvedProgram = QFileInfo(m_program).isAbsolute()
                            ? m_program
                            : QStandardPaths::findExecutable(m_program);

    m_process = new QProcess(this);
    // GUI apps often start in "/"; use somewhere writable for --output.
    m_process->setWorkingDirectory(QDir::homePath());
    connect(m_process, &QProcess::readyReadStandardOutput, this,
            [this] { read(QProcess::StandardOutput); });
    connect(m_process, &QProcess::readyReadStandardError, this,
            [this] { read(QProcess::StandardError); });
    connect(m_process, &QProcess::finished, this, &RequestRunner::finished);
    connect(m_process, &QProcess::errorOccurred, this, &RequestRunner::failedToStart);

    m_state = State::Running;
    m_elapsed.start();
    emit outputChanged(); // cleared
    emit stateChanged();
    m_process->start(m_program, arguments, QIODevice::ReadOnly);
    return true;
}

void RequestRunner::stop()
{
    if (!m_process)
        return;
    m_stopRequested = true;
    m_process->kill();
}

void RequestRunner::read(QProcess::ProcessChannel channel)
{
    if (!m_process)
        return;
    const bool isStdout = channel == QProcess::StandardOutput;
    QString chunk = isStdout ? m_outDecoder.decode(m_process->readAllStandardOutput())
                             : m_errDecoder.decode(m_process->readAllStandardError());
    if (m_truncated || chunk.isEmpty())
        return;
    if (m_output.size() + chunk.size() > kMaxChars) {
        chunk.truncate(kMaxChars - m_output.size());
        m_truncated = true;
    }
    m_output += chunk;
    (isStdout ? m_stdout : m_stderr) += chunk;
    emit outputChanged();
}

void RequestRunner::finished(int exitCode, QProcess::ExitStatus status)
{
    read(QProcess::StandardOutput);
    read(QProcess::StandardError);
    m_elapsedMs = m_elapsed.elapsed();
    m_exitCode = exitCode;
    m_state = status == QProcess::NormalExit ? State::Finished : State::Crashed;
    m_process->deleteLater();
    m_process = nullptr;
    emit stateChanged();
}

void RequestRunner::failedToStart(QProcess::ProcessError error)
{
    if (error != QProcess::FailedToStart || !m_process)
        return; // crashes and kills are reported through finished()
    m_state = State::NotFound;
    m_process->deleteLater();
    m_process = nullptr;
    emit stateChanged();
}
