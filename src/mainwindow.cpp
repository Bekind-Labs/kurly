#include "mainwindow.h"

#include "curlcommand.h"
#include "curlparser.h"
#include "importdialog.h"
#include "responseformatter.h"
#include "spacing.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDir>
#include <QComboBox>
#include <QEvent>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <vector>

namespace {

// QPlainTextEdit word-wraps very long lines in quadratic time (a 500k-char
// line takes ~20s), which minified JSON responses easily hit. Turn wrapping
// off for text like that; it's unreadable wrapped anyway.
void setTextWrappingSensibly(QPlainTextEdit *edit, const QString &text)
{
    constexpr qsizetype kMaxWrappedLine = 10000;
    qsizetype longest = 0, start = 0;
    while (start <= text.size()) {
        qsizetype end = text.indexOf(QLatin1Char('\n'), start);
        if (end < 0)
            end = text.size();
        longest = std::max(longest, end - start);
        start = end + 1;
    }
    const auto mode = longest > kMaxWrappedLine ? QPlainTextEdit::NoWrap
                                                : QPlainTextEdit::WidgetWidth;
    // Switch modes on an empty document so the old mode never lays out the new text.
    if (edit->lineWrapMode() != mode) {
        edit->clear();
        edit->setLineWrapMode(mode);
    }
    edit->setPlainText(text);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();

    // Saved choice wins; otherwise follow the system language.
    QString lang = QSettings().value(QStringLiteral("language")).toString();
    if (lang.isEmpty())
        lang = QLocale::system().language() == QLocale::Japanese ? QStringLiteral("ja")
                                                                 : QStringLiteral("en");
    const int idx = m_language->findData(lang);
    m_language->setCurrentIndex(idx < 0 ? 0 : idx);
    setLanguage(m_language->currentData().toString());

    // Window sizes are expressed in grid units, per the HIG.
    // The minimum width comes from the layout (it depends on the language);
    // the form scrolls vertically, so only a minimum height is needed.
    setMinimumHeight(20 * Spacing::gridUnit);
    resize(62 * Spacing::gridUnit, 44 * Spacing::gridUnit);
}

MainWindow::~MainWindow()
{
    // Don't let the process report back into a half-destroyed window.
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void MainWindow::buildUi()
{
    // ---- Left: the form -------------------------------------------------
    auto *form = new QWidget;
    auto *formLayout = new QVBoxLayout(form);
    // Leave room between the content and the column divider.
    formLayout->setContentsMargins(0, 0, Spacing::large, 0);
    formLayout->setSpacing(Spacing::large); // between groups

    auto *importRow = new QHBoxLayout;
    m_import = new QPushButton;
    m_import->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste")));
    importRow->addWidget(m_import);
    importRow->addStretch();
    formLayout->addLayout(importRow);

    auto *top = new QFormLayout;
    top->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    top->setContentsMargins(0, 0, 0, 0);
    top->setHorizontalSpacing(Spacing::small);
    top->setVerticalSpacing(Spacing::small);
    m_urlLabel = new QLabel;
    m_url = new QLineEdit;
    m_methodLabel = new QLabel;
    m_method = new QComboBox;
    m_method->addItems({"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"});
    top->addRow(m_urlLabel, m_url);
    top->addRow(m_methodLabel, m_method);
    formLayout->addLayout(top);

    // Headers
    m_headersBox = new QGroupBox;
    auto *hl = new QVBoxLayout(m_headersBox);
    hl->setContentsMargins(Spacing::large, Spacing::large, Spacing::large, Spacing::large);
    hl->setSpacing(Spacing::small);
    m_headers = new QTableWidget(0, 2);
    m_headers->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_headers->verticalHeader()->hide();
    m_headers->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_headers->setMinimumHeight(6 * Spacing::gridUnit);
    auto *hbtn = new QHBoxLayout;
    hbtn->setSpacing(Spacing::medium);
    m_addHeader = new QPushButton;
    m_removeHeader = new QPushButton;
    hbtn->addWidget(m_addHeader);
    hbtn->addWidget(m_removeHeader);
    hbtn->addStretch();
    hl->addWidget(m_headers);
    hl->addLayout(hbtn);
    formLayout->addWidget(m_headersBox);

    // Body
    m_bodyBox = new QGroupBox;
    auto *bl = new QVBoxLayout(m_bodyBox);
    bl->setContentsMargins(Spacing::large, Spacing::large, Spacing::large, Spacing::large);
    m_body = new QPlainTextEdit;
    m_body->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_body->setMinimumHeight(4 * Spacing::gridUnit);
    bl->addWidget(m_body);
    formLayout->addWidget(m_bodyBox, 1);

    // Auth & misc
    m_authBox = new QGroupBox;
    auto *al = new QFormLayout(m_authBox);
    al->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    al->setContentsMargins(Spacing::large, Spacing::large, Spacing::large, Spacing::large);
    al->setHorizontalSpacing(Spacing::small);
    al->setVerticalSpacing(Spacing::small);
    m_userLabel = new QLabel;
    m_user = new QLineEdit;
    m_agentLabel = new QLabel;
    m_agent = new QLineEdit;
    m_timeoutLabel = new QLabel;
    m_timeout = new QSpinBox;
    m_timeout->setRange(0, 86400);
    m_outputLabel = new QLabel;
    m_output = new QLineEdit;
    al->addRow(m_userLabel, m_user);
    al->addRow(m_agentLabel, m_agent);
    al->addRow(m_timeoutLabel, m_timeout);
    al->addRow(m_outputLabel, m_output);
    formLayout->addWidget(m_authBox);

    // Flags
    m_optionsBox = new QGroupBox;
    auto *ol = new QVBoxLayout(m_optionsBox);
    ol->setContentsMargins(Spacing::large, Spacing::large, Spacing::large, Spacing::large);
    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(Spacing::large); // columns are separate groups
    grid->setVerticalSpacing(Spacing::small);
    m_follow = new QCheckBox;
    m_insecure = new QCheckBox;
    m_verbose = new QCheckBox;
    m_silent = new QCheckBox;
    m_include = new QCheckBox;
    m_fail = new QCheckBox;
    m_compressed = new QCheckBox;
    const QList<QCheckBox *> flags{m_follow, m_insecure, m_verbose, m_silent,
                                   m_include, m_fail, m_compressed};
    for (int i = 0; i < flags.size(); ++i)
        grid->addWidget(flags[i], i % 4, i / 4);
    ol->addLayout(grid);
    formLayout->addWidget(m_optionsBox);

    // ---- Right: command (top) and response (bottom) ----------------------
    auto *commandPane = new QWidget;
    auto *rl = new QVBoxLayout(commandPane);
    // Room between the content and the dividers to the left and below.
    rl->setContentsMargins(Spacing::large, 0, 0, Spacing::large);
    rl->setSpacing(Spacing::small); // title → content, content → its controls

    auto *langRow = new QHBoxLayout;
    langRow->setSpacing(Spacing::small);
    m_languageLabel = new QLabel;
    m_language = new QComboBox;
    // Language names are shown in their own language and are not translated.
    m_language->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_language->addItem(QStringLiteral("日本語"), QStringLiteral("ja"));
    m_resultLabel = new QLabel;
    langRow->addWidget(m_resultLabel);
    langRow->addStretch();
    langRow->addWidget(m_languageLabel);
    langRow->addWidget(m_language);
    rl->addLayout(langRow);

    m_result = new QPlainTextEdit;
    m_result->setReadOnly(true);
    m_result->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    rl->addWidget(m_result, 1);

    auto *toggles = new QHBoxLayout;
    toggles->setSpacing(Spacing::large);
    m_longOpts = new QCheckBox;
    m_multiline = new QCheckBox;
    toggles->addWidget(m_longOpts);
    toggles->addWidget(m_multiline);
    toggles->addStretch();
    rl->addLayout(toggles);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(Spacing::medium);
    m_copy = new QPushButton;
    m_run = new QPushButton;
    m_run->setDefault(true);
    buttons->addWidget(m_copy, 1);
    buttons->addWidget(m_run, 1);
    rl->addLayout(buttons);

    auto *responsePane = new QWidget;
    auto *pl = new QVBoxLayout(responsePane);
    // Room between the content and the dividers to the left and above.
    pl->setContentsMargins(Spacing::large, Spacing::large, 0, 0);
    pl->setSpacing(Spacing::small);
    auto *statusRow = new QHBoxLayout;
    statusRow->setSpacing(Spacing::large);
    m_responseLabel = new QLabel;
    m_status = new QLabel;
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_pretty = new QCheckBox;
    m_pretty->setChecked(QSettings().value(QStringLiteral("prettyPrint"), true).toBool());
    m_pretty->setEnabled(false);
    statusRow->addWidget(m_responseLabel);
    statusRow->addWidget(m_pretty);
    statusRow->addStretch();
    statusRow->addWidget(m_status);
    pl->addLayout(statusRow);
    m_response = new QPlainTextEdit;
    m_response->setReadOnly(true);
    m_response->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    pl->addWidget(m_response, 1);

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(100);
    connect(m_renderTimer, &QTimer::timeout, this, &MainWindow::renderResponse);
    connect(m_pretty, &QCheckBox::toggled, this, [this](bool on) {
        QSettings().setValue(QStringLiteral("prettyPrint"), on);
        renderResponse();
    });

    auto *right = new QSplitter(Qt::Vertical);
    right->addWidget(commandPane);
    right->addWidget(responsePane);
    right->setStretchFactor(0, 1);
    right->setStretchFactor(1, 2);
    right->setChildrenCollapsible(false);
    right->setHandleWidth(Spacing::large); // gap between command and response

    // The form scrolls instead of squashing its fields when the window is small.
    auto *formScroll = m_formScroll = new QScrollArea;
    formScroll->setObjectName(QStringLiteral("formScroll"));
    formScroll->setWidget(form);
    formScroll->setWidgetResizable(true);
    formScroll->setFrameShape(QFrame::NoFrame); // frameless container: no padding
    formScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // The scroll area's viewport would otherwise paint the form in the
    // "base" (text field) color instead of the window background.
    formScroll->viewport()->setAutoFillBackground(false);
    form->setAutoFillBackground(false);

    auto *split = new QSplitter;
    split->addWidget(formScroll);
    split->addWidget(right);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 1);
    split->setChildrenCollapsible(false);
    split->setHandleWidth(Spacing::large); // gap between the two columns

    // Window edge padding.
    auto *central = new QWidget;
    auto *cl = new QVBoxLayout(central);
    cl->setContentsMargins(Spacing::large, Spacing::large, Spacing::large, Spacing::large);
    cl->addWidget(split);
    setCentralWidget(central);

    // Stable names for tests and accessibility tooling.
    m_url->setObjectName(QStringLiteral("url"));
    m_method->setObjectName(QStringLiteral("method"));
    m_headers->setObjectName(QStringLiteral("headers"));
    m_addHeader->setObjectName(QStringLiteral("addHeader"));
    m_removeHeader->setObjectName(QStringLiteral("removeHeader"));
    m_body->setObjectName(QStringLiteral("body"));
    m_user->setObjectName(QStringLiteral("user"));
    m_agent->setObjectName(QStringLiteral("userAgent"));
    m_timeout->setObjectName(QStringLiteral("timeout"));
    m_output->setObjectName(QStringLiteral("outputFile"));
    m_follow->setObjectName(QStringLiteral("followRedirects"));
    m_insecure->setObjectName(QStringLiteral("insecure"));
    m_verbose->setObjectName(QStringLiteral("verbose"));
    m_silent->setObjectName(QStringLiteral("silent"));
    m_include->setObjectName(QStringLiteral("includeHeaders"));
    m_fail->setObjectName(QStringLiteral("failOnError"));
    m_compressed->setObjectName(QStringLiteral("compressed"));
    m_longOpts->setObjectName(QStringLiteral("longOptions"));
    m_multiline->setObjectName(QStringLiteral("multiline"));
    m_result->setObjectName(QStringLiteral("command"));
    m_copy->setObjectName(QStringLiteral("copy"));
    m_run->setObjectName(QStringLiteral("run"));
    m_response->setObjectName(QStringLiteral("response"));
    m_pretty->setObjectName(QStringLiteral("pretty"));
    m_status->setObjectName(QStringLiteral("status"));
    m_language->setObjectName(QStringLiteral("language"));
    m_import->setObjectName(QStringLiteral("import"));

    // ---- Wiring ---------------------------------------------------------
    for (auto *e : {m_url, m_user, m_agent, m_output})
        connect(e, &QLineEdit::textChanged, this, &MainWindow::updateCommand);
    connect(m_body, &QPlainTextEdit::textChanged, this, &MainWindow::updateCommand);
    connect(m_method, &QComboBox::currentTextChanged, this, &MainWindow::updateCommand);
    connect(m_timeout, &QSpinBox::valueChanged, this, &MainWindow::updateCommand);
    connect(m_headers, &QTableWidget::itemChanged, this, &MainWindow::updateCommand);
    for (auto *c : {m_follow, m_insecure, m_verbose, m_silent, m_include, m_fail,
                    m_compressed, m_longOpts, m_multiline})
        connect(c, &QCheckBox::toggled, this, &MainWindow::updateCommand);

    connect(m_addHeader, &QPushButton::clicked, this, [this] { addHeaderRow(); });
    connect(m_removeHeader, &QPushButton::clicked, this, &MainWindow::removeSelectedHeaders);
    connect(m_copy, &QPushButton::clicked, this, &MainWindow::copyCommand);
    connect(m_run, &QPushButton::clicked, this, &MainWindow::runOrStop);
    connect(m_import, &QPushButton::clicked, this, &MainWindow::importCommand);
    auto *importShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I), this);
    connect(importShortcut, &QShortcut::activated, this, &MainWindow::importCommand);
    // Ctrl+Enter (Cmd+Return on macOS) runs from anywhere, even inside the body editor.
    auto *runShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(runShortcut, &QShortcut::activated, this, [this] {
        if (m_run->isEnabled() && m_runState != RunState::Running)
            runOrStop();
    });
    connect(m_language, &QComboBox::currentIndexChanged, this, [this] {
        setLanguage(m_language->currentData().toString());
    });

    // A starter header row for convenience.
    addHeaderRow();
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("Kurly – curl command builder"));

    m_import->setText(tr("Import curl command…"));
    m_import->setToolTip(tr("Fill in the form from an existing curl command, such as one "
                            "copied from a browser's developer tools (%1)")
                             .arg(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I)
                                      .toString(QKeySequence::NativeText)));
    m_urlLabel->setText(tr("URL:"));
    m_url->setPlaceholderText(tr("https://example.com/api"));
    m_methodLabel->setText(tr("Method:"));

    m_headersBox->setTitle(tr("Headers"));
    m_headers->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
    m_addHeader->setText(tr("Add header"));
    m_removeHeader->setText(tr("Remove selected"));

    m_bodyBox->setTitle(tr("Request body"));
    m_body->setPlaceholderText(tr("Data to send (e.g. JSON)"));

    m_authBox->setTitle(tr("Authentication and other"));
    m_userLabel->setText(tr("User (user:password):"));
    m_agentLabel->setText(tr("User agent:"));
    m_timeoutLabel->setText(tr("Timeout:"));
    m_timeout->setSpecialValueText(tr("None"));
    m_timeout->setSuffix(tr(" s"));
    m_outputLabel->setText(tr("Save output to file:"));
    m_output->setToolTip(tr("When run from Kurly, relative paths are saved in your home folder."));

    m_optionsBox->setTitle(tr("Options"));
    m_follow->setText(tr("Follow redirects"));
    m_insecure->setText(tr("Skip TLS certificate verification"));
    m_verbose->setText(tr("Verbose output"));
    m_silent->setText(tr("Silent mode"));
    m_include->setText(tr("Include response headers"));
    m_fail->setText(tr("Fail on HTTP errors"));
    m_compressed->setText(tr("Request compressed response"));

    m_languageLabel->setText(tr("Language:"));
    m_resultLabel->setText(tr("Command:"));
    m_result->setPlaceholderText(tr("Enter a URL to generate the command."));
    m_longOpts->setText(tr("Use long option names"));
    m_multiline->setText(tr("Split across multiple lines"));
    m_copy->setText(tr("Copy command"));
    m_responseLabel->setText(tr("Response:"));
    m_pretty->setText(tr("Format JSON/XML"));
    m_pretty->setToolTip(tr("Show JSON and XML responses indented. Available once a request "
                            "finishes with a complete JSON or XML response."));
    m_response->setPlaceholderText(tr("Press Run to send the request. curl's output appears here."));
    updateRunState();
    renderResponse(); // e.g. the "[Output truncated]" note

    // The form only scrolls vertically, so the column must stay wide enough
    // for its widest row. Labels change width with the language.
    m_formScroll->widget()->adjustSize();
    m_formScroll->setMinimumWidth(m_formScroll->widget()->minimumSizeHint().width()
                                  + m_formScroll->verticalScrollBar()->sizeHint().width());
}

void MainWindow::setLanguage(const QString &code)
{
    qApp->removeTranslator(&m_translator);
    if (code != QLatin1String("en")
        && m_translator.load(QStringLiteral(":/i18n/kurly_%1.qm").arg(code))) {
        qApp->installTranslator(&m_translator);
    }
    QSettings().setValue(QStringLiteral("language"), code);
    // Installing/removing a translator posts LanguageChange to widgets; we
    // also retranslate directly so the first call at startup is covered.
    retranslateUi();
    updateCommand();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange && m_copy)
        retranslateUi();
    QMainWindow::changeEvent(event);
}

void MainWindow::addHeaderRow(const QString &name, const QString &value)
{
    QSignalBlocker block(m_headers);
    const int row = m_headers->rowCount();
    m_headers->insertRow(row);
    m_headers->setItem(row, 0, new QTableWidgetItem(name));
    m_headers->setItem(row, 1, new QTableWidgetItem(value));
    block.unblock();
    m_headers->setCurrentCell(row, 0);
    updateCommand();
}

void MainWindow::removeSelectedHeaders()
{
    QList<int> rows;
    for (const auto &idx : m_headers->selectionModel()->selectedRows())
        rows << idx.row();
    if (rows.isEmpty() && m_headers->currentRow() >= 0)
        rows << m_headers->currentRow();
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
        m_headers->removeRow(r);
    updateCommand();
}

CurlOptions MainWindow::currentOptions() const
{
    CurlOptions o;
    o.url = m_url->text();
    o.method = m_method->currentText();
    for (int r = 0; r < m_headers->rowCount(); ++r) {
        const auto *n = m_headers->item(r, 0);
        const auto *v = m_headers->item(r, 1);
        if (n && !n->text().trimmed().isEmpty())
            o.headers.append({n->text(), v ? v->text() : QString()});
    }
    o.body = m_body->toPlainText();
    o.user = m_user->text();
    o.userAgent = m_agent->text();
    o.outputFile = m_output->text();
    o.maxTimeSeconds = m_timeout->value();
    o.followRedirects = m_follow->isChecked();
    o.insecure = m_insecure->isChecked();
    o.verbose = m_verbose->isChecked();
    o.silent = m_silent->isChecked();
    o.includeHeaders = m_include->isChecked();
    o.failOnError = m_fail->isChecked();
    o.compressed = m_compressed->isChecked();
    o.longOptions = m_longOpts->isChecked();
    o.multiline = m_multiline->isChecked();
    return o;
}

void MainWindow::updateCommand()
{
    const QString cmd = CurlCommand::build(currentOptions());
    if (cmd != m_result->toPlainText())
        setTextWrappingSensibly(m_result, cmd);
    m_copy->setEnabled(!cmd.isEmpty());
    updateRunState();
}

void MainWindow::copyCommand()
{
    QApplication::clipboard()->setText(m_result->toPlainText());
    m_copy->setText(tr("Copied!"));
    QTimer::singleShot(1500, this, [this] { m_copy->setText(tr("Copy command")); });
}

// ---- Running the request ---------------------------------------------------

void MainWindow::runOrStop()
{
    if (m_process) {
        m_stopRequested = true;
        m_process->kill();
        return;
    }

    QStringList args = CurlCommand::arguments(currentOptions());
    if (args.isEmpty())
        return;
    // Without a terminal, curl prints a progress table to stderr, which would
    // clutter the output pane. This flag hides it but still shows errors.
    args.prepend(QStringLiteral("--no-progress-meter"));

    m_rawOutput.clear();
    m_stdout.clear();
    m_stderr.clear();
    m_truncated = false;
    m_stopRequested = false;
    m_outDecoder.resetState();
    m_errDecoder.resetState();
    renderResponse();

    m_process = new QProcess(this);
    // GUI apps often start in "/"; use somewhere writable for --output.
    m_process->setWorkingDirectory(QDir::homePath());
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::readStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &MainWindow::readStderr);
    connect(m_process, &QProcess::finished, this, &MainWindow::processFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MainWindow::processError);

    m_runState = RunState::Running;
    m_elapsed.start();
    updateRunState();
    m_process->start(m_curlProgram, args, QIODevice::ReadOnly);
}

void MainWindow::readStdout()
{
    if (m_process)
        appendOutput(m_outDecoder.decode(m_process->readAllStandardOutput()), true);
}

void MainWindow::readStderr()
{
    if (m_process)
        appendOutput(m_errDecoder.decode(m_process->readAllStandardError()), false);
}

void MainWindow::appendOutput(const QString &text, bool isStdout)
{
    // Keep memory and the UI in check if someone downloads something large.
    constexpr qsizetype kMaxChars = 2 * 1024 * 1024;
    if (m_truncated || text.isEmpty())
        return;
    QString chunk = text;
    if (m_rawOutput.size() + chunk.size() > kMaxChars) {
        chunk.truncate(kMaxChars - m_rawOutput.size());
        m_truncated = true;
    }
    m_rawOutput += chunk;
    (isStdout ? m_stdout : m_stderr) += chunk;
    // Redraw at most every 100ms rather than per chunk: re-laying out a
    // long line for every chunk gets quadratically slow.
    if (!m_renderTimer->isActive())
        m_renderTimer->start();
}

void MainWindow::renderResponse()
{
    m_renderTimer->stop();
    QString text = m_rawOutput;
    // Format once curl is done, if the whole response arrived.
    const bool done = m_runState != RunState::Running;
    m_pretty->setEnabled(false);
    if (done && !m_truncated && !m_stdout.isEmpty()) {
        const auto formatted = ResponseFormatter::format(m_stdout);
        if (formatted.kind != ResponseFormatter::Kind::None) {
            m_pretty->setEnabled(true);
            if (m_pretty->isChecked()) {
                // curl's own messages (errors, -v output) come first.
                text = m_stderr + formatted.text;
            }
        }
    }
    if (m_truncated)
        text += QLatin1Char('\n') + tr("[Output truncated]") + QLatin1Char('\n');

    if (text == m_response->toPlainText())
        return;
    auto *bar = m_response->verticalScrollBar();
    const bool follow = !done && bar->value() == bar->maximum();
    setTextWrappingSensibly(m_response, text);
    if (follow)
        bar->setValue(bar->maximum());
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus status)
{
    readStdout();
    readStderr();
    m_elapsedMs = m_elapsed.elapsed();
    m_exitCode = exitCode;
    m_runState = status == QProcess::NormalExit ? RunState::Finished : RunState::Crashed;
    m_process->deleteLater();
    m_process = nullptr;
    updateRunState();
    renderResponse();
}

void MainWindow::processError(QProcess::ProcessError error)
{
    if (error != QProcess::FailedToStart || !m_process)
        return; // crashes/kills are reported through finished()
    m_runState = RunState::NotFound;
    m_process->deleteLater();
    m_process = nullptr;
    updateRunState();
}

QString MainWindow::exitCodeHint(int code) const
{
    switch (code) {
    case 3:  return tr("malformed URL");
    case 6:  return tr("could not resolve host");
    case 7:  return tr("could not connect to server");
    case 22: return tr("HTTP error returned");
    case 28: return tr("timed out");
    case 35: return tr("TLS/SSL connection error");
    case 47: return tr("too many redirects");
    case 52: return tr("server returned nothing");
    case 60: return tr("TLS certificate problem");
    default: return {};
    }
}

void MainWindow::updateRunState()
{
    if (!m_run || !m_status)
        return;

    const bool running = m_runState == RunState::Running;
    m_run->setText(running ? tr("Stop") : tr("Run"));
    m_run->setEnabled(running || !m_result->toPlainText().isEmpty());

    QString text;
    QString color;
    switch (m_runState) {
    case RunState::Idle:
        break;
    case RunState::Running:
        text = tr("Running…");
        break;
    case RunState::Finished: {
        const QString time = tr("%1 ms").arg(m_elapsedMs);
        if (m_exitCode == 0) {
            text = tr("Done · %1").arg(time);
            color = QStringLiteral("#2e7d32");
        } else {
            const QString hint = exitCodeHint(m_exitCode);
            text = hint.isEmpty() ? tr("curl exited with code %1 · %2").arg(m_exitCode).arg(time)
                                  : tr("curl exited with code %1 (%2) · %3")
                                        .arg(m_exitCode).arg(hint, time);
            color = QStringLiteral("#c62828");
        }
        break;
    }
    case RunState::Crashed:
        text = m_stopRequested ? tr("Stopped") : tr("curl stopped unexpectedly");
        color = m_stopRequested ? QString() : QStringLiteral("#c62828");
        break;
    case RunState::NotFound:
        text = tr("Could not start curl. Is it installed and on your PATH?");
        color = QStringLiteral("#c62828");
        break;
    }
    m_status->setText(text);
    m_status->setStyleSheet(color.isEmpty() ? QString()
                                            : QStringLiteral("color: %1;").arg(color));
}

// ---- Importing an existing command -------------------------------------------

void MainWindow::importCommand()
{
    ImportDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const auto &result = dialog.result();
    loadOptions(result.options);

    if (!result.ignored.isEmpty()) {
        QMessageBox box(QMessageBox::Information, tr("Import curl Command"),
                        tr("The command was imported, but Kurly doesn't support some of "
                           "its options, so they were skipped:"),
                        QMessageBox::Ok, this);
        box.setInformativeText(result.ignored.join(QLatin1Char('\n')));
        box.exec();
    }
}

void MainWindow::loadOptions(const CurlOptions &o)
{
    // Block per-field updates; rebuild the command once at the end.
    std::vector<QSignalBlocker> blockers;
    for (QObject *w : std::initializer_list<QObject *>{
             m_url, m_method, m_headers, m_body, m_user, m_agent, m_output, m_timeout,
             m_follow, m_insecure, m_verbose, m_silent, m_include, m_fail, m_compressed})
        blockers.emplace_back(w);

    m_url->setText(o.url);
    int idx = m_method->findText(o.method);
    if (idx < 0) {
        m_method->addItem(o.method);
        idx = m_method->count() - 1;
    }
    m_method->setCurrentIndex(idx);

    m_headers->setRowCount(0);
    for (const auto &h : o.headers) {
        const int row = m_headers->rowCount();
        m_headers->insertRow(row);
        m_headers->setItem(row, 0, new QTableWidgetItem(h.first));
        m_headers->setItem(row, 1, new QTableWidgetItem(h.second));
    }

    m_body->setPlainText(o.body);
    m_user->setText(o.user);
    m_agent->setText(o.userAgent);
    m_output->setText(o.outputFile);
    m_timeout->setValue(o.maxTimeSeconds);
    m_follow->setChecked(o.followRedirects);
    m_insecure->setChecked(o.insecure);
    m_verbose->setChecked(o.verbose);
    m_silent->setChecked(o.silent);
    m_include->setChecked(o.includeHeaders);
    m_fail->setChecked(o.failOnError);
    m_compressed->setChecked(o.compressed);

    blockers.clear();
    if (o.headers.isEmpty())
        addHeaderRow(); // also refreshes the command
    else
        updateCommand();
}
