#include <QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "importdialog.h"
#include "librarypanel.h"
#include "mainwindow.h"
#include "requeststore.h"
#include "testserver.h"

// The request library: the sidebar, switching requests, and persistence
// across restarts.
class TestLibrary : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    MainWindow *w = nullptr;

    QString libraryPath() const { return m_dir.filePath("library.json"); }

    template<typename T>
    T *find(const char *name) const
    {
        auto *child = w->findChild<T *>(QString::fromLatin1(name));
        if (!child)
            qFatal("no child named %s", name);
        return child;
    }
    QListWidget *list() const { return find<QListWidget>("libraryList"); }
    QLineEdit *url() const { return find<QLineEdit>("url"); }
    QString status() const { return find<QLabel>("status")->text(); }
    QString response() const { return find<QPlainTextEdit>("response")->toPlainText(); }
    QStringList names() const
    {
        QStringList out;
        for (int i = 0; i < list()->count(); ++i)
            out << list()->item(i)->text();
        return out;
    }
    void select(int row) { list()->setCurrentRow(row); }
    void restart()
    {
        delete w; // saves pending changes
        w = new MainWindow(nullptr, libraryPath());
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }

    static bool haveCurl() { return !QStandardPaths::findExecutable("curl").isEmpty(); }

    // Runs `interact` against the next modal dialog once it is open.
    template<typename Dialog, typename F>
    static void onNextModal(F interact)
    {
        QTimer::singleShot(0, qApp, [interact] {
            if (auto *d = qobject_cast<Dialog *>(QApplication::activeModalWidget()))
                interact(d);
            else
                onNextModal<Dialog>(interact);
        });
    }
    static void clickButton(QMessageBox *box, const QString &text)
    {
        for (auto *b : box->buttons())
            if (b->text() == text) {
                b->click();
                return;
            }
        qFatal("no button %s", qPrintable(text));
    }

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName("Bekind Labs Tests");
        QCoreApplication::setApplicationName("tst_library");
        QStandardPaths::setTestModeEnabled(true);
        qputenv("no_proxy", "127.0.0.1,localhost");
        qputenv("NO_PROXY", "127.0.0.1,localhost");
        for (const char *var : {"http_proxy", "HTTP_PROXY", "all_proxy", "ALL_PROXY"})
            qunsetenv(var);
    }

    void init()
    {
        QSettings().clear();
        QSettings().setValue("language", "en");
        QFile::remove(libraryPath());
        w = new MainWindow(nullptr, libraryPath());
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }

    void cleanup()
    {
        delete w;
        w = nullptr;
    }

    // ---- Showing the library ------------------------------------------------------

    void hiddenByDefault()
    {
        QVERIFY(!w->library()->isVisible());
        QVERIFY(!find<QPushButton>("libraryToggle")->isChecked());
    }

    void toggleIsRemembered()
    {
        find<QPushButton>("libraryToggle")->click();
        QVERIFY(w->library()->isVisible());
        QCOMPARE(QSettings().value("libraryVisible").toBool(), true);
        restart();
        QVERIFY(w->library()->isVisible());
        QVERIFY(find<QPushButton>("libraryToggle")->isChecked());

        QVERIFY(QTest::qWaitForWindowActive(w));
        QTest::keyClick(w, Qt::Key_B, Qt::ControlModifier);
        QVERIFY(!w->library()->isVisible());
        QVERIFY(!find<QPushButton>("libraryToggle")->isChecked());
    }

    // ---- Persistence ----------------------------------------------------------------

    void editsSurviveRestart()
    {
        // Even with the library hidden, the request is kept.
        url()->setText("https://keep.test/a");
        find<QComboBox>("method")->setCurrentText("PUT");
        find<QPlainTextEdit>("body")->setPlainText("hello");
        find<QCheckBox>("followRedirects")->setChecked(true);
        restart();
        QCOMPARE(url()->text(), QString("https://keep.test/a"));
        QCOMPARE(find<QComboBox>("method")->currentText(), QString("PUT"));
        QCOMPARE(find<QPlainTextEdit>("body")->toPlainText(), QString("hello"));
        QVERIFY(find<QCheckBox>("followRedirects")->isChecked());
    }

    void displayPreferencesSurviveRestart()
    {
        find<QCheckBox>("longOptions")->setChecked(true);
        find<QCheckBox>("multiline")->setChecked(true);
        restart();
        QVERIFY(find<QCheckBox>("longOptions")->isChecked());
        QVERIFY(find<QCheckBox>("multiline")->isChecked());
    }

    void selectionSurvivesRestart()
    {
        url()->setText("https://one.test/");
        w->library()->newRequest();
        url()->setText("https://two.test/");
        select(0);
        QCOMPARE(url()->text(), QString("https://one.test/"));
        restart();
        QCOMPARE(url()->text(), QString("https://one.test/"));
        QCOMPARE(list()->currentRow(), 0);
    }

    // ---- Working with requests ---------------------------------------------------------

    void newRequestAndSwitching()
    {
        url()->setText("https://api.test/users");
        QCOMPARE(names(), QStringList{"GET api.test/users"}); // automatic name follows the URL

        QVERIFY(QTest::qWaitForWindowActive(w));
        QTest::keyClick(w, Qt::Key_N, Qt::ControlModifier);
        QVERIFY(w->library()->isVisible()); // shown, so the new request is visible
        QCOMPARE(list()->count(), 2);
        QCOMPARE(list()->currentRow(), 1);
        QVERIFY(url()->text().isEmpty());
        QVERIFY(url()->hasFocus());
        QCOMPARE(names()[1], QString("New request"));
        QVERIFY(find<QPlainTextEdit>("command")->toPlainText().isEmpty());

        url()->setText("https://api.test/orders");
        find<QComboBox>("method")->setCurrentText("POST");
        QCOMPARE(names(), (QStringList{"GET api.test/users", "POST api.test/orders"}));

        select(0);
        QCOMPARE(url()->text(), QString("https://api.test/users"));
        QCOMPARE(find<QComboBox>("method")->currentText(), QString("GET"));
        select(1);
        QCOMPARE(url()->text(), QString("https://api.test/orders"));
    }

    void renaming()
    {
        url()->setText("https://api.test/users");
        QListWidgetItem *item = list()->item(0);
        item->setText("  My users  ");
        QCOMPARE(w->store()->requests()[0].name, QString("My users"));
        QCOMPARE(item->text(), QString("My users"));
        url()->setText("https://api.test/other"); // a custom name doesn't follow the URL
        QCOMPARE(item->text(), QString("My users"));

        item->setText(""); // clearing the name brings the automatic one back
        QCOMPARE(w->store()->requests()[0].name, QString());
        QCOMPARE(item->text(), QString("GET api.test/other"));

        item->setText("GET api.test/other"); // typing the automatic name keeps it automatic
        QCOMPARE(w->store()->requests()[0].name, QString());
    }

    void duplicating()
    {
        url()->setText("https://api.test/users");
        find<QCheckBox>("verbose")->setChecked(true);
        find<QPushButton>("libraryDuplicate")->click();
        QCOMPARE(list()->count(), 2);
        QCOMPARE(list()->currentRow(), 1);
        QCOMPARE(url()->text(), QString("https://api.test/users"));
        QVERIFY(find<QCheckBox>("verbose")->isChecked());
        // The copy is independent.
        url()->setText("https://api.test/copy");
        select(0);
        QCOMPARE(url()->text(), QString("https://api.test/users"));
    }

    void deletingAsksFirst()
    {
        url()->setText("https://keep.test/");
        w->library()->newRequest();
        url()->setText("https://delete.test/");

        onNextModal<QMessageBox>([](QMessageBox *box) {
            QVERIFY(box->text().contains("GET delete.test"));
            clickButton(box, "Cancel");
        });
        find<QPushButton>("libraryDelete")->click();
        QCOMPARE(list()->count(), 2);

        onNextModal<QMessageBox>([](QMessageBox *box) { clickButton(box, "Delete"); });
        find<QPushButton>("libraryDelete")->click();
        QCOMPARE(list()->count(), 1);
        QCOMPARE(url()->text(), QString("https://keep.test/")); // its neighbour is shown

        // Deleting the last request leaves an empty one.
        onNextModal<QMessageBox>([](QMessageBox *box) { clickButton(box, "Delete"); });
        find<QPushButton>("libraryDelete")->click();
        QCOMPARE(list()->count(), 1);
        QVERIFY(url()->text().isEmpty());
    }

    void filtering()
    {
        url()->setText("https://api.test/users");
        w->library()->newRequest();
        url()->setText("https://api.test/orders");
        w->library()->newRequest();
        url()->setText("https://other.test/");
        auto *filter = find<QLineEdit>("libraryFilter");

        filter->setText("ORDERS"); // case-insensitive, matches name or URL
        QVERIFY(list()->item(0)->isHidden());
        QVERIFY(!list()->item(1)->isHidden());
        QVERIFY(list()->item(2)->isHidden());
        QCOMPARE(list()->dragDropMode(), QAbstractItemView::NoDragDrop);

        filter->clear();
        for (int i = 0; i < 3; ++i)
            QVERIFY(!list()->item(i)->isHidden());
        QCOMPARE(list()->dragDropMode(), QAbstractItemView::InternalMove);

        // Adding a request clears the filter so it can be seen.
        filter->setText("orders");
        w->library()->newRequest();
        QVERIFY(filter->text().isEmpty());
    }

    void reordering()
    {
        url()->setText("https://a.test/");
        w->library()->newRequest();
        url()->setText("https://b.test/");
        w->library()->newRequest();
        url()->setText("https://c.test/");
        // What a drag and drop does to the list.
        QVERIFY(list()->model()->moveRow(QModelIndex(), 2, QModelIndex(), 0));
        QCOMPARE(names(), (QStringList{"GET c.test", "GET a.test", "GET b.test"}));
        QCOMPARE(RequestStore::autoName(w->store()->requests()[0].options), QString("GET c.test"));
        restart();
        QCOMPARE(names(), (QStringList{"GET c.test", "GET a.test", "GET b.test"}));
    }

    void keyboardShortcuts()
    {
        find<QPushButton>("libraryToggle")->click();
        url()->setText("https://keys.test/");
        list()->setFocus();
        QTest::keyClick(list(), Qt::Key_F2);
        // An inline editor for the name now has focus.
        auto *editor = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(editor && list()->isAncestorOf(editor));
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Escape);

        w->library()->newRequest();
        list()->setFocus();
        bool asked = false;
        onNextModal<QMessageBox>([&asked](QMessageBox *box) {
            asked = true;
            clickButton(box, "Delete");
        });
        QTest::keyClick(list(), Qt::Key_Delete);
        QVERIFY(asked);
        QCOMPARE(list()->count(), 1);
    }

    void contextMenu()
    {
        find<QPushButton>("libraryToggle")->click();
        url()->setText("https://menu.test/");
        QStringList entries;
        QTimer::singleShot(0, this, [&entries] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            for (QAction *a : menu->actions())
                entries << (a->isSeparator() ? QString("-") : a->text());
            menu->actions().at(1)->trigger(); // Duplicate
            menu->close();
        });
        const QRect rect = list()->visualItemRect(list()->item(0));
        emit list()->customContextMenuRequested(rect.center());
        QCOMPARE(entries, (QStringList{"Rename", "Duplicate", "-", "Delete"}));
        QCOMPARE(list()->count(), 2);
        // Right-clicking empty space shows nothing.
        emit list()->customContextMenuRequested(QPoint(5, list()->viewport()->height() - 5));
        QVERIFY(!QApplication::activePopupWidget());
    }

    // ---- Import ------------------------------------------------------------------

    void importWithLibraryCreatesRequest()
    {
        find<QPushButton>("libraryToggle")->click();
        QVERIFY(w->library()->isVisible());
        url()->setText("https://existing.test/");
        QApplication::clipboard()->setText("curl -X DELETE https://imported.test/x");
        onNextModal<ImportDialog>([](ImportDialog *d) { d->accept(); });
        find<QPushButton>("import")->click();
        QCOMPARE(list()->count(), 2);
        QCOMPARE(list()->currentRow(), 1);
        QCOMPARE(url()->text(), QString("https://imported.test/x"));
        select(0);
        QCOMPARE(url()->text(), QString("https://existing.test/")); // untouched

        // An empty current request is filled in instead.
        w->library()->newRequest();
        QApplication::clipboard()->setText("curl https://into-empty.test/");
        onNextModal<ImportDialog>([](ImportDialog *d) { d->accept(); });
        find<QPushButton>("import")->click();
        QCOMPARE(list()->count(), 3);
        QCOMPARE(url()->text(), QString("https://into-empty.test/"));
    }

    void importWithoutLibraryReplaces()
    {
        url()->setText("https://existing.test/");
        QApplication::clipboard()->setText("curl https://imported.test/");
        onNextModal<ImportDialog>([](ImportDialog *d) { d->accept(); });
        find<QPushButton>("import")->click();
        QCOMPARE(w->store()->count(), 1);
        QCOMPARE(url()->text(), QString("https://imported.test/"));
    }

    // ---- Responses per request -------------------------------------------------------

    void eachRequestKeepsItsResponse()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));

        url()->setText(server.url("/first"));
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().startsWith("Done"), 10000);
        QVERIFY(response().contains("GET /first"));

        w->library()->newRequest();
        QVERIFY(response().isEmpty()); // a new request has no response yet
        QVERIFY(status().isEmpty());
        url()->setText(server.url("/second"));
        find<QPushButton>("run")->click();
        QTRY_VERIFY_WITH_TIMEOUT(status().startsWith("Done"), 10000);
        QVERIFY(response().contains("GET /second"));

        select(0);
        QVERIFY(response().contains("GET /first"));
        QVERIFY(status().startsWith("Done"));
    }

    void runningContinuesInBackground()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer hang;
        hang.mode = TestServer::Mode::Hang;
        QVERIFY(hang.listen(QHostAddress::LocalHost));

        url()->setText(hang.url());
        find<QPushButton>("run")->click();
        QCOMPARE(status(), QString("Running…"));

        w->library()->newRequest();
        QVERIFY(status().isEmpty());
        QCOMPARE(find<QPushButton>("run")->text(), QString("Run"));

        select(0);
        QCOMPARE(status(), QString("Running…")); // still going
        QCOMPARE(find<QPushButton>("run")->text(), QString("Stop"));
        find<QPushButton>("run")->click();
        QTRY_COMPARE_WITH_TIMEOUT(status(), QString("Stopped"), 10000);
    }

    void deletingRunningRequestStopsIt()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer hang;
        hang.mode = TestServer::Mode::Hang;
        QVERIFY(hang.listen(QHostAddress::LocalHost));
        url()->setText("https://keep.test/");
        w->library()->newRequest();
        url()->setText(hang.url());
        find<QPushButton>("run")->click();
        QCOMPARE(status(), QString("Running…"));
        onNextModal<QMessageBox>([](QMessageBox *box) { clickButton(box, "Delete"); });
        find<QPushButton>("libraryDelete")->click();
        QCOMPARE(url()->text(), QString("https://keep.test/"));
        QVERIFY(status().isEmpty());
    }

    // ---- Problems with the library file ----------------------------------------------

    void damagedLibraryIsReported()
    {
        delete w;
        w = nullptr;
        QFile f(libraryPath());
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{ not json");
        f.close();

        QString message;
        onNextModal<QMessageBox>([&message](QMessageBox *box) {
            message = box->text();
            box->accept();
        });
        w = new MainWindow(nullptr, libraryPath());
        w->show();
        QTRY_VERIFY(!message.isEmpty());
        QVERIFY2(message.contains(".corrupt-"), qPrintable(message));
        url()->setText("https://works.test/"); // and Kurly carries on with a new library
        QCOMPARE(w->store()->requests()[0].options.url, QString("https://works.test/"));
    }

    void newerLibraryIsReadOnly()
    {
        delete w;
        w = nullptr;
        const QByteArray future = "{\"version\": 99, \"requests\": [{\"id\": \"x\", \"url\": \"https://future.test/\"}]}";
        QFile f(libraryPath());
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(future);
        f.close();

        QString message;
        onNextModal<QMessageBox>([&message](QMessageBox *box) {
            message = box->text();
            box->accept();
        });
        w = new MainWindow(nullptr, libraryPath());
        w->show();
        QTRY_VERIFY(!message.isEmpty());
        QVERIFY(message.contains("newer version"));
        QCOMPARE(url()->text(), QString("https://future.test/"));
        url()->setText("https://changed.test/");
        bool warnedAgain = false; // still read-only on the next launch
        onNextModal<QMessageBox>([&warnedAgain](QMessageBox *box) {
            warnedAgain = true;
            box->accept();
        });
        restart();
        QTRY_VERIFY(warnedAgain);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), future); // left alone
    }

    void unreadableLibraryIsReported()
    {
#ifdef Q_OS_WIN
        QSKIP("Permissions can't make a file unreadable here");
#else
        delete w;
        w = nullptr;
        QFile f(libraryPath());
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{}");
        f.close();
        QFile::setPermissions(libraryPath(), QFileDevice::Permissions());
        if (QFile(libraryPath()).open(QIODevice::ReadOnly)) {
            QFile::setPermissions(libraryPath(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            QSKIP("Running as a user who can read any file (e.g. root)");
        }
        QString message;
        onNextModal<QMessageBox>([&message](QMessageBox *box) {
            message = box->text();
            box->accept();
        });
        w = new MainWindow(nullptr, libraryPath());
        w->show();
        QTRY_VERIFY(!message.isEmpty());
        QVERIFY(message.contains("permission"));
        QFile::setPermissions(libraryPath(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif
    }

    void saveFailureIsReportedOnce()
    {
        delete w;
        w = nullptr;
        // A file where the library's folder should be makes saving impossible.
        QFile blocker(m_dir.filePath("blocked"));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        w = new MainWindow(nullptr, m_dir.filePath("blocked/library.json"));
        w->show();

        int shown = 0;
        onNextModal<QMessageBox>([&shown](QMessageBox *box) {
            ++shown;
            box->accept();
        });
        url()->setText("https://a.test/");
        QTRY_COMPARE(shown, 1);
        url()->setText("https://b.test/");
        QTest::qWait(700); // past the save delay
        QCOMPARE(shown, 1);
    }

    // ---- Language ----------------------------------------------------------------------

    void libraryIsTranslated()
    {
        auto *language = find<QComboBox>("language");
        language->setCurrentIndex(language->findData("ja"));
        QCoreApplication::processEvents();
        QCOMPARE(find<QPushButton>("libraryToggle")->text(), QString::fromUtf8("ライブラリ"));
        QCOMPARE(find<QPushButton>("libraryNew")->text(), QString::fromUtf8("新規"));
        QCOMPARE(names()[0], QString::fromUtf8("新しいリクエスト"));
        language->setCurrentIndex(language->findData("en"));
        QCoreApplication::processEvents();
        QCOMPARE(names()[0], QString("New request"));
    }
};

QTEST_MAIN(TestLibrary)
#include "tst_library.moc"
