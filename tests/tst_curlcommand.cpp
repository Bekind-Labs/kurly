#include <QtTest>

#include "curlcommand.h"

class TestCurlCommand : public QObject
{
    Q_OBJECT
private slots:
    void quoting_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        QTest::newRow("safe") << "abc-1.2/x" << "abc-1.2/x";
        QTest::newRow("space") << "a b" << "'a b'";
        QTest::newRow("quote") << "it's" << "'it'\\''s'";
        QTest::newRow("empty") << "" << "''";
        QTest::newRow("japanese") << QString::fromUtf8("日本語") << QString::fromUtf8("'日本語'");
    }
    void quoting()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QCOMPARE(CurlCommand::shellQuote(in), out);
    }

    void emptyUrl() { QVERIFY(CurlCommand::build({}).isEmpty()); }

    void simpleGet()
    {
        CurlOptions o;
        o.url = "https://example.com/?a=1&b=2";
        QCOMPARE(CurlCommand::build(o), QString("curl 'https://example.com/?a=1&b=2'"));
    }

    void postWithBodyOmitsX()
    {
        CurlOptions o;
        o.url = "https://example.com";
        o.method = "POST";
        o.body = "{\"a\":1}";
        o.headers << qMakePair(QString("Content-Type"), QString("application/json"));
        QCOMPARE(CurlCommand::build(o),
                 QString("curl -H 'Content-Type: application/json' "
                         "--data-raw '{\"a\":1}' https://example.com"));
    }

    void methodIsAlwaysWhatWasChosen()
    {
        CurlOptions o;
        o.url = "https://example.com";
        o.body = "a=1";
        o.method = "GET"; // a body alone would make curl send POST
        QCOMPARE(CurlCommand::build(o), QString("curl -X GET --data-raw a=1 https://example.com"));
        o.method = "PUT";
        QCOMPARE(CurlCommand::build(o), QString("curl -X PUT --data-raw a=1 https://example.com"));
        o.body.clear();
        o.method = "POST"; // without a body, POST must be explicit
        QCOMPARE(CurlCommand::build(o), QString("curl -X POST https://example.com"));
    }

    void putAndHead()
    {
        CurlOptions o;
        o.url = "https://example.com";
        o.method = "PUT";
        QCOMPARE(CurlCommand::build(o), QString("curl -X PUT https://example.com"));
        o.method = "HEAD";
        QCOMPARE(CurlCommand::build(o), QString("curl -I https://example.com"));
    }

    void longOptionsAndFlags()
    {
        CurlOptions o;
        o.url = "https://example.com";
        o.followRedirects = true;
        o.insecure = true;
        o.compressed = true;
        o.maxTimeSeconds = 5;
        o.user = "me:secret pw";
        o.longOptions = true;
        QCOMPARE(CurlCommand::build(o),
                 QString("curl --location --insecure --compressed --max-time 5 "
                         "--user 'me:secret pw' https://example.com"));
    }

    void argumentsAreUnquoted()
    {
        CurlOptions o;
        o.url = "https://example.com/?a=1&b=2";
        o.method = "PATCH";
        o.headers << qMakePair(QString("X-Name"), QString("it's me"));
        QCOMPARE(CurlCommand::arguments(o),
                 (QStringList{"-X", "PATCH", "-H", "X-Name: it's me",
                              "https://example.com/?a=1&b=2"}));
        QVERIFY(CurlCommand::arguments({}).isEmpty());
    }

    void cookies()
    {
        CurlOptions o;
        o.url = "https://example.com";
        o.headers = {{"Accept", "*/*"}};
        o.cookies = {{"session", "abc 123"}, {" ", "nameless is skipped"}, {" theme ", " dark "}};
        QCOMPARE(CurlCommand::build(o),
                 QString("curl -H 'Accept: */*' -b 'session=abc 123; theme=dark' https://example.com"));
        o.longOptions = true;
        QVERIFY(CurlCommand::build(o).contains("--cookie 'session=abc 123; theme=dark'"));
        o.cookies = {{" ", "x"}};
        QVERIFY(!CurlCommand::build(o).contains("cookie"));
    }

    void splitCookies()
    {
        QList<QPair<QString, QString>> out;
        QVERIFY(CurlCommand::splitCookies(" a=1;b = x=y ;; c= ", &out));
        QCOMPARE(out, (QList<QPair<QString, QString>>{{"a", "1"}, {"b", "x=y"}, {"c", ""}}));
        // Not name=value pairs: nothing is added.
        for (const char *bad : {"", " ; ", "cookies.txt", "a=1; =2", "a=1; junk"})
            QVERIFY2(!CurlCommand::splitCookies(bad, &out), bad);
        QCOMPARE(out.size(), 3);
    }

    void multiline()
    {
        CurlOptions o;
        o.url = "https://example.com";
        o.silent = true;
        o.multiline = true;
        QCOMPARE(CurlCommand::build(o),
                 QString("curl \\\n  -s \\\n  https://example.com"));
    }
};

QTEST_APPLESS_MAIN(TestCurlCommand)
#include "tst_curlcommand.moc"
