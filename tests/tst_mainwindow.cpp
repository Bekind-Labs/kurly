#include <QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "curlcommand.h"
#include "importdialog.h"
#include "librarypanel.h"
#include "mainwindow.h"

#include "testserver.h"


class TestMainWindow : public QObject
{
    Q_OBJECT

    MainWindow *w = nullptr;
    QTemporaryDir m_dir; // each test gets a fresh, private request library
    QString libraryPath() const { return m_dir.filePath("library.json"); }

    template<typename T>
    T *find(const char *name) const
    {
        auto *child = w->findChild<T *>(QString::fromLatin1(name));
        if (!child)
            qFatal("no child named %s", name);
        return child;
    }
    QString command() const { return find<QPlainTextEdit>("command")->toPlainText(); }
    QString status() const { return find<QLabel>("status")->text(); }
    QString response() const { return find<QPlainTextEdit>("response")->toPlainText(); }

    static bool haveCurl() { return !QStandardPaths::findExecutable("curl").isEmpty(); }

    // Runs `interact` against the next modal dialog once it is open.
    template<typename Dialog, typename F>
    static void onNextModal(F interact)
    {
        QTimer::singleShot(0, qApp, [interact] {
            if (auto *d = qobject_cast<Dialog *>(QApplication::activeModalWidget()))
                interact(d);
            else
                onNextModal<Dialog>(interact); // not open yet; try again
        });
    }

private slots:
    void initTestCase()
    {
        // Keep tests away from the user's real settings.
        QCoreApplication::setOrganizationName("Bekind Labs Tests");
        QCoreApplication::setApplicationName("tst_mainwindow");
        QStandardPaths::setTestModeEnabled(true); // never touch the real library
        // Make sure curl talks to the local test server directly.
        for (const char *var : {"http_proxy", "HTTP_PROXY", "https_proxy", "HTTPS_PROXY",
                                "all_proxy", "ALL_PROXY"})
            qunsetenv(var);
        qputenv("no_proxy", "127.0.0.1,localhost");
        qputenv("NO_PROXY", "127.0.0.1,localhost");
    }

    void init()
    {
        QSettings().clear();
        QFile::remove(libraryPath());
        QSettings().setValue("language", "en");
        w = new MainWindow(nullptr, libraryPath());
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }

    void cleanup()
    {
        delete w;
        w = nullptr;
    }

    // ---- Building ----------------------------------------------------------

    void initialState()
    {
        QVERIFY(command().isEmpty());
        QVERIFY(!find<QPushButton>("copy")->isEnabled());
        QVERIFY(!find<QPushButton>("run")->isEnabled());
        QCOMPARE(find<QTableWidget>("headers")->rowCount(), 1);
        QVERIFY(status().isEmpty());
        QCOMPARE(find<QComboBox>("method")->currentText(), QString("GET"));
    }

    void typingUrlBuildsCommand()
    {
        QTest::keyClicks(find<QLineEdit>("url"), "https://example.com/?a=1&b=2");
        QCOMPARE(command(), QString("curl 'https://example.com/?a=1&b=2'"));
        QVERIFY(find<QPushButton>("copy")->isEnabled());
        QVERIFY(find<QPushButton>("run")->isEnabled());

        find<QLineEdit>("url")->clear();
        QVERIFY(command().isEmpty());
        QVERIFY(!find<QPushButton>("run")->isEnabled());
    }

    void checkboxesAddFlags_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("shortFlag");
        QTest::addColumn<QString>("longFlag");
        QTest::newRow("follow") << "followRedirects" << "-L" << "--location";
        QTest::newRow("insecure") << "insecure" << "-k" << "--insecure";
        QTest::newRow("verbose") << "verbose" << "-v" << "--verbose";
        QTest::newRow("silent") << "silent" << "-s" << "--silent";
        QTest::newRow("include") << "includeHeaders" << "-i" << "--include";
        QTest::newRow("fail") << "failOnError" << "-f" << "--fail";
        QTest::newRow("compressed") << "compressed" << "--compressed" << "--compressed";
    }
    void checkboxesAddFlags()
    {
        QFETCH(QString, name);
        QFETCH(QString, shortFlag);
        QFETCH(QString, longFlag);
        find<QLineEdit>("url")->setText("https://x.test");
        auto *box = w->findChild<QCheckBox *>(name);
        QVERIFY(box);
        box->click();
        QCOMPARE(command(), "curl " + shortFlag + " https://x.test");
        find<QCheckBox>("longOptions")->click();
        QCOMPARE(command(), "curl " + longFlag + " https://x.test");
        box->click();
        QCOMPARE(command(), QString("curl https://x.test"));
    }

    void fieldsAddOptions()
    {
        find<QLineEdit>("url")->setText("https://x.test");
        find<QComboBox>("method")->setCurrentText("PUT");
        find<QLineEdit>("user")->setText("me:pw");
        find<QLineEdit>("userAgent")->setText("kurly test");
        find<QLineEdit>("outputFile")->setText("out.json");
        find<QSpinBox>("timeout")->setValue(9);
        find<QPlainTextEdit>("body")->setPlainText("a=1");
        QCOMPARE(command(), QString("curl -X PUT --max-time 9 -u me:pw -A 'kurly test' "
                                    "--data-raw a=1 -o out.json https://x.test"));

        find<QCheckBox>("multiline")->click();
        QVERIFY(command().contains(" \\\n  -X PUT \\\n"));
    }

    void headersAddEditRemove()
    {
        auto *table = find<QTableWidget>("headers");
        find<QLineEdit>("url")->setText("https://x.test");
        table->item(0, 0)->setText("X-One");
        table->item(0, 1)->setText("1");
        QCOMPARE(command(), QString("curl -H 'X-One: 1' https://x.test"));

        find<QPushButton>("addHeader")->click();
        QCOMPARE(table->rowCount(), 2);
        QCOMPARE(table->currentRow(), 1);
        table->item(1, 0)->setText("X-Two");
        table->item(1, 1)->setText("2");
        QCOMPARE(command(), QString("curl -H 'X-One: 1' -H 'X-Two: 2' https://x.test"));

        // Rows without a name are ignored.
        find<QPushButton>("addHeader")->click();
        table->item(2, 1)->setText("orphan value");
        QCOMPARE(command(), QString("curl -H 'X-One: 1' -H 'X-Two: 2' https://x.test"));

        // Remove a selected row...
        table->clearSelection();
        table->selectRow(0);
        find<QPushButton>("removeHeader")->click();
        QCOMPARE(table->rowCount(), 2);
        QCOMPARE(command(), QString("curl -H 'X-Two: 2' https://x.test"));

        // ...or, with nothing selected, the current row.
        table->clearSelection();
        table->setCurrentCell(0, 0, QItemSelectionModel::NoUpdate);
        find<QPushButton>("removeHeader")->click();
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(command(), QString("curl https://x.test"));
    }

    void loadOptionsRoundTrip()
    {
        CurlOptions o;
        o.url = "https://example.com/v1?x=1";
        o.method = "PATCH";
        o.headers = {{"Accept", "application/json"}, {"Cookie", "a=1; b=2"}};
        o.body = "{\n  \"a\": 1\n}";
        o.user = "me:pw";
        o.userAgent = "UA";
        o.outputFile = "out.bin";
        o.maxTimeSeconds = 30;
        o.followRedirects = o.insecure = o.verbose = o.silent = true;
        o.includeHeaders = o.failOnError = o.compressed = true;
        w->loadOptions(o);
        QCOMPARE(w->currentOptions(), o);
        QCOMPARE(command(), CurlCommand::build(o));

        // Loading a request without headers leaves one blank row to type into,
        // and clears everything the previous request set.
        CurlOptions plain;
        plain.url = "https://plain.test";
        w->loadOptions(plain);
        QCOMPARE(w->currentOptions(), plain);
        QCOMPARE(find<QTableWidget>("headers")->rowCount(), 1);
    }

    void loadOptionsCustomMethod()
    {
        CurlOptions o;
        o.url = "https://x.test";
        o.method = "PURGE";
        auto *method = find<QComboBox>("method");
        const int before = method->count();
        w->loadOptions(o);
        QCOMPARE(method->currentText(), QString("PURGE"));
        QCOMPARE(method->count(), before + 1);
        w->loadOptions(o); // not added twice
        QCOMPARE(method->count(), before + 1);
        QCOMPARE(command(), QString("curl -X PURGE https://x.test"));
    }

    void copyButton()
    {
        find<QLineEdit>("url")->setText("https://x.test");
        auto *copy = find<QPushButton>("copy");
        copy->click();
        QCOMPARE(QApplication::clipboard()->text(), QString("curl https://x.test"));
        QCOMPARE(copy->text(), QString("Copied!"));
        QTRY_COMPARE_WITH_TIMEOUT(copy->text(), QString("Copy command"), 3000);
    }

    // ---- Languages ---------------------------------------------------------

    void switchLanguage()
    {
        auto *language = find<QComboBox>("language");
        auto *copy = find<QPushButton>("copy");
        QCOMPARE(copy->text(), QString("Copy command"));

        language->setCurrentIndex(language->findData("ja"));
        QCoreApplication::processEvents(); // deliver the LanguageChange events too
        QCOMPARE(copy->text(), QString::fromUtf8("コマンドをコピー"));
        QCOMPARE(find<QPushButton>("run")->text(), QString::fromUtf8("実行"));
        QVERIFY(w->windowTitle().contains(QString::fromUtf8("コマンドビルダー")));
        QCOMPARE(QSettings().value("language").toString(), QString("ja"));

        // Qt's own strings (standard buttons, context menus) follow too.
        if (QFile::exists(":/i18n/qtbase_ja.qm")) {
            QCOMPARE(QCoreApplication::translate("QLineEdit", "&Copy"), QString::fromUtf8("コピー(&C)"));
            QCOMPARE(QCoreApplication::translate("QPlatformTheme", "Cancel"), QString::fromUtf8("キャンセル"));
        } else
            qWarning("qtbase_ja.qm not embedded; skipping the Qt strings check");

        language->setCurrentIndex(language->findData("en"));
        QCOMPARE(copy->text(), QString("Copy command"));
        QCOMPARE(QCoreApplication::translate("QLineEdit", "&Copy"), QString("&Copy"));
        QCOMPARE(QSettings().value("language").toString(), QString("en"));
    }

    void rememberedLanguage()
    {
        QSettings().setValue("language", "ja");
        MainWindow second(nullptr, m_dir.filePath("second.json"));
        QCOMPARE(second.findChild<QComboBox *>("language")->currentData().toString(), QString("ja"));
        QCOMPARE(second.findChild<QPushButton *>("copy")->text(), QString::fromUtf8("コマンドをコピー"));
    }

    void systemLanguageByDefault()
    {
        QSettings().remove("language");
        MainWindow second(nullptr, m_dir.filePath("second.json"));
        // QLocale::system() can't be faked portably, so check that a
        // supported language matching it was picked and remembered.
        const QString expected = QLocale::system().language() == QLocale::Japanese ? "ja" : "en";
        const QString lang = second.findChild<QComboBox *>("language")->currentData().toString();
        QCOMPARE(lang, expected);
        QCOMPARE(QSettings().value("language").toString(), lang);
    }

    // ---- Importing ---------------------------------------------------------

    void importReplacesForm()
    {
        find<QCheckBox>("verbose")->click(); // should be cleared by the import
        QApplication::clipboard()->clear();
        onNextModal<ImportDialog>([](ImportDialog *d) {
            d->findChild<QPlainTextEdit *>("input")->setPlainText(
                "curl 'https://api.test/items' -H 'accept: application/json' "
                "--data-raw '{\"a\":1}' --compressed");
            d->accept();
        });
        find<QPushButton>("import")->click();

        QCOMPARE(find<QLineEdit>("url")->text(), QString("https://api.test/items"));
        QCOMPARE(find<QComboBox>("method")->currentText(), QString("POST"));
        QCOMPARE(find<QPlainTextEdit>("body")->toPlainText(), QString("{\"a\":1}"));
        QVERIFY(find<QCheckBox>("compressed")->isChecked());
        QVERIFY(!find<QCheckBox>("verbose")->isChecked());
        auto *table = find<QTableWidget>("headers");
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 0)->text(), QString("accept"));
        QCOMPARE(command(), QString("curl --compressed -H 'accept: application/json' "
                                    "--data-raw '{\"a\":1}' https://api.test/items"));
    }

    void importReportsSkippedOptions()
    {
        QApplication::clipboard()->clear();
        QString reported;
        onNextModal<ImportDialog>([&reported](ImportDialog *d) {
            d->findChild<QPlainTextEdit *>("input")->setPlainText(
                "curl --proxy http://p -F a=b https://api.test/");
            onNextModal<QMessageBox>([&reported](QMessageBox *box) {
                reported = box->informativeText();
                box->accept();
            });
            d->accept();
        });
        find<QPushButton>("import")->click();
        QCOMPARE(reported, QString("--proxy\n-F"));
        QCOMPARE(find<QLineEdit>("url")->text(), QString("https://api.test/"));
    }

    void importCancelKeepsForm()
    {
        find<QLineEdit>("url")->setText("https://keep.test");
        QApplication::clipboard()->setText("curl https://other.test");
        onNextModal<ImportDialog>([](ImportDialog *d) { d->reject(); });
        find<QPushButton>("import")->click();
        QCOMPARE(find<QLineEdit>("url")->text(), QString("https://keep.test"));
    }

    void importShortcut()
    {
        QVERIFY(QTest::qWaitForWindowActive(w));
        QApplication::clipboard()->setText("curl https://shortcut.test");
        onNextModal<ImportDialog>([](ImportDialog *d) { d->accept(); }); // pre-filled
        QTest::keyClick(w, Qt::Key_I, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(find<QLineEdit>("url")->text(), QString("https://shortcut.test"));
    }

    // ---- Running -----------------------------------------------------------

    void runEchoesRequest()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));

        CurlOptions o;
        o.url = server.url();
        o.method = "POST";
        o.headers = {{"X-Kurly", QString::fromUtf8("テスト")}};
        o.body = QString::fromUtf8("hello 日本語");
        w->loadOptions(o);

        auto *run = find<QPushButton>("run");
        run->click();
        QCOMPARE(run->text(), QString("Stop"));
        QTRY_VERIFY_WITH_TIMEOUT(status().startsWith("Done"), 10000);
        QCOMPARE(run->text(), QString("Run"));
        QVERIFY2(response().contains("POST /echo"), qPrintable(response()));
        QVERIFY(response().contains(QString::fromUtf8("X-Kurly: テスト")));
        QVERIFY(response().contains(QString::fromUtf8("body=hello 日本語")));
        // The progress meter is suppressed, so there's no stderr noise.
        QVERIFY(!response().contains("% Total"));
    }

    void crashNamesTheCurlThatRan()
    {
#ifdef Q_OS_WIN
        QSKIP("Uses a shell script as a stand-in for curl");
#else
        // A "curl" that kills itself, as a crashing curl would.
        QTemporaryDir dir;
        const QString fake = dir.filePath("curl");
        QFile f(fake);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("#!/bin/sh\nkill -SEGV $$\n");
        f.close();
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

        w->setCurlProgram(fake);
        find<QLineEdit>("url")->setText("https://x.test");
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().contains("stopped unexpectedly"), 10000);
        QCOMPARE(status(), QString("curl (%1) stopped unexpectedly").arg(fake));
        QCOMPARE(find<QLabel>("status")->toolTip(), QString("Ran %1").arg(fake));
#endif
    }

    void runShortcut()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        QVERIFY(QTest::qWaitForWindowActive(w));
        QTest::keyClick(w, Qt::Key_Return, Qt::ControlModifier);
        QTRY_VERIFY_WITH_TIMEOUT(status().startsWith("Done"), 10000);
        QVERIFY(response().contains("GET /echo"));
    }

    void runReportsHttpError()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::NotFound;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        find<QCheckBox>("failOnError")->click();
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().contains("code 22"), 10000);
        QVERIFY(status().contains("HTTP error returned"));
    }

    void runReportsConnectionFailure()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        // Find a port nothing is listening on.
        quint16 port;
        {
            QTcpServer probe;
            QVERIFY(probe.listen(QHostAddress::LocalHost));
            port = probe.serverPort();
        }
        find<QLineEdit>("url")->setText(QString("http://127.0.0.1:%1/").arg(port));
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().contains("code 7"), 10000);
        QVERIFY(status().contains("could not connect"));
        QVERIFY(response().contains("curl: (7)"));
    }

    void runReportsTimeout()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::Hang;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        find<QSpinBox>("timeout")->setValue(1);
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().contains("code 28"), 10000);
        QVERIFY(status().contains("timed out"));
    }

    void runReportsMalformedUrl()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        find<QLineEdit>("url")->setText("http://[");
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().contains("code 3 "), 10000);
        QVERIFY(status().contains("malformed URL"));
    }

    void runExitCodeWithoutHint()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        // An unsupported protocol (exit code 1) has no friendly hint.
        find<QLineEdit>("url")->setText("kurlytest://x");
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().contains("curl exited with code 1 ·"), 10000);
    }

    void stopRunningRequest()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::Hang;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        auto *run = find<QPushButton>("run");
        run->click();
        QCOMPARE(status(), QString("Running…"));
        QVERIFY(run->isEnabled());
        run->click(); // Stop
        QTRY_COMPARE_WITH_TIMEOUT(status(), QString("Stopped"), 10000);
        QCOMPARE(run->text(), QString("Run"));
    }

    void largeOutputIsTruncated()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::Big;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        find<QPushButton>("run")->click();
        QElapsedTimer timer;
        timer.start();
        QTRY_VERIFY_WITH_TIMEOUT(status().startsWith("Done"), 20000);
        QVERIFY(response().endsWith("[Output truncated]\n"));
        QVERIFY(response().size() < TestServer::kBigSize);
        // Regression check: this used to freeze the UI for minutes.
        QVERIFY2(timer.elapsed() < 10000, qPrintable(QString::number(timer.elapsed())));
        QVERIFY(!find<QCheckBox>("pretty")->isEnabled()); // incomplete: not formatted
        // A huge line isn't word-wrapped (that's what was slow)...
        QCOMPARE(find<QPlainTextEdit>("response")->lineWrapMode(), QPlainTextEdit::NoWrap);
        // ...and wrapping comes back for normal output.
        w->setCurlProgram("kurly-no-such-curl-program");
        find<QPushButton>("run")->click();
        QTRY_VERIFY(status().contains("Could not start curl"));
        QCOMPARE(find<QPlainTextEdit>("response")->lineWrapMode(), QPlainTextEdit::WidgetWidth);
    }

    // ---- Formatting --------------------------------------------------------

    void formatsJsonResponse()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::Json;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        auto *pretty = find<QCheckBox>("pretty");
        QVERIFY(pretty->isChecked()); // on by default
        QVERIFY(!pretty->isEnabled()); // nothing to format yet

        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().startsWith("Done"), 10000);
        QVERIFY(pretty->isEnabled());
        QCOMPARE(response(), QString::fromUtf8(TestServer::kJsonPretty));
        QCOMPARE(find<QPlainTextEdit>("response")->lineWrapMode(), QPlainTextEdit::WidgetWidth);

        // Toggle to the raw response and back; the choice is remembered.
        pretty->click();
        QCOMPARE(response(), QString::fromUtf8(TestServer::kJson));
        QCOMPARE(QSettings().value("prettyPrint").toBool(), false);
        pretty->click();
        QCOMPARE(response(), QString::fromUtf8(TestServer::kJsonPretty));
        QCOMPARE(QSettings().value("prettyPrint").toBool(), true);
    }

    void formatsJsonWithHeadersAndVerbose()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::Json;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        find<QCheckBox>("includeHeaders")->click();
        find<QCheckBox>("verbose")->click();
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().startsWith("Done"), 10000);
        // curl's verbose log first, then the headers, then the formatted body.
        const QString text = response();
        QVERIFY2(text.startsWith("*"), qPrintable(text));
        const int headers = text.indexOf("HTTP/1.1 200 OK");
        QVERIFY(headers > 0);
        QVERIFY(text.indexOf("> GET /echo") < headers);
        QVERIFY(text.endsWith(QString::fromUtf8(TestServer::kJsonPretty)));
    }

    void plainResponseIsNotFormatted()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().startsWith("Done"), 10000);
        QVERIFY(!find<QCheckBox>("pretty")->isEnabled());
        QVERIFY(response().startsWith("request:\nGET /echo"));
    }

    void prettySettingIsRemembered()
    {
        QSettings().setValue("prettyPrint", false);
        MainWindow second(nullptr, m_dir.filePath("second.json"));
        QVERIFY(!second.findChild<QCheckBox *>("pretty")->isChecked());
    }

    // ---- Layout ------------------------------------------------------------

    void panesPadTheirOwnContent()
    {
        // The dividers run edge to edge (no window margin)...
        auto *split = qobject_cast<QSplitter *>(w->centralWidget());
        QVERIFY(split);
        QCOMPARE(split->orientation(), Qt::Horizontal);
        auto *right = split->findChild<QSplitter *>();
        QVERIFY(right);
        QCOMPARE(right->orientation(), Qt::Vertical);
        QCOMPARE(right->height(), split->height()); // full height
        // ...and every pane keeps its content 8px (HIG largeSpacing) from
        // the window edges and the dividers.
        const QMargins padding(8, 8, 8, 8);
        QCOMPARE(w->library()->layout()->contentsMargins(), padding);
        QCOMPARE(find<QScrollArea>("formScroll")->widget()->layout()->contentsMargins(), padding);
        QCOMPARE(find<QPlainTextEdit>("command")->parentWidget()->layout()->contentsMargins(), padding);
        QCOMPARE(find<QPlainTextEdit>("response")->parentWidget()->layout()->contentsMargins(), padding);
    }

    void buttonRowsLineUp_data()
    {
        QTest::addColumn<QString>("lang");
        QTest::addColumn<QString>("font");
        // A font without Japanese characters makes Japanese labels use a
        // fallback font with other metrics (as when the system font isn't
        // available), which used to give buttons in a row different heights.
        for (const char *lang : {"en", "ja"})
            for (const char *font : {"", "DejaVu Sans"})
                QTest::addRow("%s, %s", lang, *font ? font : "default font") << lang << font;
    }
    void buttonRowsLineUp()
    {
        QFETCH(QString, lang);
        QFETCH(QString, font);
        const QFont before = QApplication::font();
        if (!font.isEmpty())
            QApplication::setFont(QFont(font, 10));
        auto *language = find<QComboBox>("language");
        language->setCurrentIndex(language->findData(lang));
        if (!find<QPushButton>("libraryToggle")->isChecked())
            find<QPushButton>("libraryToggle")->click();
        QCoreApplication::processEvents();
        const QList<QStringList> rows = {
            {"libraryToggle", "import"},
            {"copy", "run"},
            {"addHeader", "removeHeader"},
            {"libraryNew", "libraryDuplicate", "libraryDelete"},
        };
        // Different fonts give labels different heights (e.g. Japanese text
        // drawn with a fallback font); simulate that on one button per row.
        for (const QStringList &row : rows) {
            auto *taller = w->findChild<QPushButton *>(row.last());
            QFont f = taller->font();
            f.setPointSizeF(f.pointSizeF() * 1.4);
            taller->setFont(f);
        }
        QCoreApplication::processEvents();
        w->resize(w->width() + 1, w->height()); // lay out again
        QCoreApplication::processEvents();
        for (const QStringList &row : rows) {
            QList<int> heights, tops;
            for (const QString &name : row) {
                auto *b = w->findChild<QPushButton *>(name);
                QVERIFY2(b, qPrintable(name));
                heights << b->height();
                tops << b->mapTo(w, QPoint()).y();
            }
            QVERIFY2(std::adjacent_find(heights.begin(), heights.end(), std::not_equal_to<>()) == heights.end(),
                     qPrintable(row.join(", ") + ": heights differ"));
            QVERIFY2(std::adjacent_find(tops.begin(), tops.end(), std::not_equal_to<>()) == tops.end(),
                     qPrintable(row.join(", ") + ": tops differ"));
        }
        QApplication::setFont(before);
    }

    void formScrollsWhenSmall()
    {
        auto *scroll = find<QScrollArea>("formScroll");
        auto *table = find<QTableWidget>("headers");
        auto *language = find<QComboBox>("language");
        auto *commandPane = find<QPlainTextEdit>("command")->parentWidget();
        for (const char *lang : {"en", "ja"}) {
            language->setCurrentIndex(language->findData(lang));
            w->resize(1, 1); // Qt clamps this to the smallest allowed size
            QTRY_VERIFY(scroll->verticalScrollBar()->maximum() > 0);
            // Nothing is squashed or cut off.
            QVERIFY2(scroll->widget()->width() >= scroll->widget()->minimumSizeHint().width(), lang);
            QVERIFY2(commandPane->width() >= commandPane->minimumSizeHint().width(), lang);
            QVERIFY(table->height() >= table->minimumHeight());
            QVERIFY(find<QPlainTextEdit>("body")->height()
                    >= find<QPlainTextEdit>("body")->minimumHeight());
        }

        w->resize(1600, 1400);
        QTRY_COMPARE(scroll->verticalScrollBar()->maximum(), 0);
    }

    void missingCurl()
    {
        w->setCurlProgram("kurly-no-such-curl-program");
        find<QLineEdit>("url")->setText("https://x.test");
        find<QPushButton>("run")->click();
        QTRY_VERIFY(status().contains("Could not start curl"));
        QCOMPARE(find<QPushButton>("run")->text(), QString("Run"));
    }

    void statusColorsFollowTheTheme()
    {
        w->setCurlProgram("kurly-no-such-curl-program");
        find<QLineEdit>("url")->setText("https://x.test");
        find<QPushButton>("run")->click();
        QTRY_VERIFY(status().contains("Could not start curl"));
        auto *label = find<QLabel>("status");
        QCOMPARE(label->styleSheet(), QString("color: #c62828;")); // dark red on light

        // Switching to a dark theme (as the system does) lightens it.
        QPalette dark = w->palette();
        dark.setColor(QPalette::Window, QColor(50, 50, 50));
        w->setPalette(dark);
        QCOMPARE(label->styleSheet(), QString("color: #ef9a9a;"));
    }

    void statusIsRetranslated()
    {
        w->setCurlProgram("kurly-no-such-curl-program");
        find<QLineEdit>("url")->setText("https://x.test");
        find<QPushButton>("run")->click();
        QTRY_VERIFY(status().contains("Could not start curl"));
        auto *language = find<QComboBox>("language");
        language->setCurrentIndex(language->findData("ja"));
        QVERIFY(status().contains(QString::fromUtf8("curlを起動できません")));
    }

    void closingWhileRunning()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::Hang;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        find<QLineEdit>("url")->setText(server.url());
        find<QPushButton>("run")->click();
        QCOMPARE(status(), QString("Running…"));
        delete w; // must kill curl cleanly, without crashing
        w = nullptr;
    }
};

QTEST_MAIN(TestMainWindow)
#include "tst_mainwindow.moc"
