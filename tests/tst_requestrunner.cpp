#include <QtTest>

#include <QSignalSpy>
#include <QStandardPaths>
#include <QScopeGuard>
#include <QTemporaryDir>

#include "requestrunner.h"
#include "testserver.h"

class TestRequestRunner : public QObject
{
    Q_OBJECT

    static bool haveCurl() { return !QStandardPaths::findExecutable("curl").isEmpty(); }

private slots:
    void initTestCase()
    {
        qputenv("no_proxy", "127.0.0.1,localhost");
        qputenv("NO_PROXY", "127.0.0.1,localhost");
        for (const char *var : {"http_proxy", "HTTP_PROXY", "all_proxy", "ALL_PROXY"})
            qunsetenv(var);
    }

    void idleUntilStarted()
    {
        RequestRunner r;
        QCOMPARE(r.state(), RequestRunner::State::Idle);
        QVERIFY(!r.start({})); // nothing to run
        r.stop();              // nothing to stop
        QCOMPARE(r.state(), RequestRunner::State::Idle);
    }

    void runsAndSeparatesOutput()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        RequestRunner r;
        QSignalSpy state(&r, &RequestRunner::stateChanged);
        QVERIFY(r.start({"-v", server.url("/x")}));
        QVERIFY(r.isRunning());
        QVERIFY(!r.start({server.url()})); // one run at a time
        QTRY_COMPARE_WITH_TIMEOUT(r.state(), RequestRunner::State::Finished, 10000);
        QCOMPARE(r.exitCode(), 0);
        QVERIFY(r.standardOutput().contains("GET /x"));
        QVERIFY(r.standardError().contains("> GET /x")); // -v goes to stderr
        QCOMPARE(r.output().size(), r.standardOutput().size() + r.standardError().size());
        QVERIFY(!r.isTruncated());
        QCOMPARE(state.count(), 2); // started, finished
        QCOMPARE(r.resolvedProgram(), QStandardPaths::findExecutable("curl"));

        // A second run starts from scratch.
        QVERIFY(r.start({server.url("/y")}));
        QTRY_COMPARE_WITH_TIMEOUT(r.state(), RequestRunner::State::Finished, 10000);
        QVERIFY(!r.output().contains("/x"));
    }

    void stopping()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::Hang;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        RequestRunner r;
        QVERIFY(r.start({server.url()}));
        r.stop();
        QTRY_COMPARE_WITH_TIMEOUT(r.state(), RequestRunner::State::Crashed, 10000);
        QVERIFY(r.stopRequested());
    }

    void missingProgram()
    {
        RequestRunner r;
        r.setProgram("kurly-no-such-curl-program");
        QVERIFY(r.start({"https://x.test"}));
        QTRY_COMPARE(r.state(), RequestRunner::State::NotFound);
        QVERIFY(!r.isRunning());
        QVERIFY(r.resolvedProgram().isEmpty());
    }

    void outputDirectory()
    {
        // Outside a Flatpak, relative output paths go to the home folder.
        qunsetenv("FLATPAK_ID");
        QCOMPARE(RequestRunner::outputDirectory(), QDir::homePath());
    }

    void outputDirectoryInFlatpak()
    {
#ifndef Q_OS_LINUX
        QSKIP("Flatpak and the XDG user directories are Linux-only");
#else
        // A temporary home whose desktop settings put Downloads somewhere known.
        QTemporaryDir home;
        const QByteArray oldHome = qgetenv("HOME"), oldConfig = qgetenv("XDG_CONFIG_HOME");
        const auto restore = qScopeGuard([&] { // even if a check below fails
            qunsetenv("FLATPAK_ID");
            qputenv("HOME", oldHome);
            if (oldConfig.isEmpty())
                qunsetenv("XDG_CONFIG_HOME");
            else
                qputenv("XDG_CONFIG_HOME", oldConfig);
        });
        qputenv("HOME", home.path().toLocal8Bit());
        qputenv("XDG_CONFIG_HOME", (home.path() + "/.config").toLocal8Bit());
        QDir(home.path()).mkpath(".config");
        QFile dirs(home.filePath(".config/user-dirs.dirs"));
        QVERIFY(dirs.open(QIODevice::WriteOnly));
        dirs.write("XDG_DOWNLOAD_DIR=\"$HOME/Kurly Downloads\"\n");
        dirs.close();
        const QString downloads = home.filePath("Kurly Downloads");

        // Inside a Flatpak only Downloads is writable...
        qputenv("FLATPAK_ID", "io.github.Bekind_Labs.Kurly");
        QCOMPARE(RequestRunner::outputDirectory(), home.path()); // ...but it doesn't exist yet
        QVERIFY(QDir().mkpath(downloads));
        QCOMPARE(RequestRunner::outputDirectory(), downloads);

        // And curl really runs there.
        if (haveCurl()) {
            TestServer server;
            QVERIFY(server.listen(QHostAddress::LocalHost));
            RequestRunner r;
            QVERIFY(r.start({"-o", "kurly-test-output.txt", server.url()}));
            QTRY_COMPARE_WITH_TIMEOUT(r.state(), RequestRunner::State::Finished, 10000);
            QVERIFY(QFile::exists(downloads + "/kurly-test-output.txt"));
        }
#endif
    }

    void destroyingWhileRunning()
    {
        if (!haveCurl())
            QSKIP("curl is not installed");
        TestServer server;
        server.mode = TestServer::Mode::Hang;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        auto *r = new RequestRunner;
        QVERIFY(r->start({server.url()}));
        delete r; // kills curl without crashing
    }
};

QTEST_GUILESS_MAIN(TestRequestRunner)
#include "tst_requestrunner.moc"
