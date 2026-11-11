#include <QtTest>

#include <QSignalSpy>
#include <QStandardPaths>

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
