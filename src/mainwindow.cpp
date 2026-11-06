#include "mainwindow.h"

#include "curlcommand.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
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
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

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

    resize(1000, 640);
}

void MainWindow::buildUi()
{
    // ---- Left: the form -------------------------------------------------
    auto *form = new QWidget;
    auto *formLayout = new QVBoxLayout(form);

    auto *top = new QFormLayout;
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
    m_headers = new QTableWidget(0, 2);
    m_headers->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_headers->verticalHeader()->hide();
    m_headers->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_headers->setMinimumHeight(110);
    auto *hbtn = new QHBoxLayout;
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
    m_body = new QPlainTextEdit;
    m_body->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_body->setMinimumHeight(80);
    bl->addWidget(m_body);
    formLayout->addWidget(m_bodyBox, 1);

    // Auth & misc
    m_authBox = new QGroupBox;
    auto *al = new QFormLayout(m_authBox);
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
    auto *grid = new QGridLayout;
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

    // ---- Right: result --------------------------------------------------
    auto *right = new QWidget;
    auto *rl = new QVBoxLayout(right);

    auto *langRow = new QHBoxLayout;
    m_languageLabel = new QLabel;
    m_language = new QComboBox;
    // Language names are shown in their own language and are not translated.
    m_language->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_language->addItem(QStringLiteral("日本語"), QStringLiteral("ja"));
    langRow->addStretch();
    langRow->addWidget(m_languageLabel);
    langRow->addWidget(m_language);
    rl->addLayout(langRow);

    m_resultLabel = new QLabel;
    rl->addWidget(m_resultLabel);
    m_result = new QPlainTextEdit;
    m_result->setReadOnly(true);
    m_result->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    rl->addWidget(m_result, 1);

    m_longOpts = new QCheckBox;
    m_multiline = new QCheckBox;
    rl->addWidget(m_longOpts);
    rl->addWidget(m_multiline);

    m_copy = new QPushButton;
    m_copy->setDefault(true);
    rl->addWidget(m_copy);

    auto *split = new QSplitter;
    split->addWidget(form);
    split->addWidget(right);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 1);
    split->setChildrenCollapsible(false);
    setCentralWidget(split);

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
    connect(m_language, &QComboBox::currentIndexChanged, this, [this] {
        setLanguage(m_language->currentData().toString());
    });

    // A starter header row for convenience.
    addHeaderRow();
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("Kurly – curl command builder"));

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

void MainWindow::updateCommand()
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

    const QString cmd = CurlCommand::build(o);
    m_result->setPlainText(cmd);
    m_copy->setEnabled(!cmd.isEmpty());
}

void MainWindow::copyCommand()
{
    QApplication::clipboard()->setText(m_result->toPlainText());
    m_copy->setText(tr("Copied!"));
    QTimer::singleShot(1500, this, [this] { m_copy->setText(tr("Copy command")); });
}
