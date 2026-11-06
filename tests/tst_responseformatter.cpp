#include <QtTest>

#include "responseformatter.h"

using ResponseFormatter::Kind;

class TestResponseFormatter : public QObject
{
    Q_OBJECT
private slots:
    void json_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        QTest::newRow("object keeps key order")
            << "{\"z\":1,\"a\":[true,false,null],\"m\":{\"k\":\"v\"}}"
            << "{\n  \"z\": 1,\n  \"a\": [\n    true,\n    false,\n    null\n  ],\n"
               "  \"m\": {\n    \"k\": \"v\"\n  }\n}";
        QTest::newRow("empty containers") << "{\"a\":{},\"b\":[ ]}"
                                          << "{\n  \"a\": {},\n  \"b\": []\n}";
        QTest::newRow("strings untouched")
            << "[\"a, b: {c}\", \"q\\\"uo\\\\te\", \"\\u65e5\"]"
            << "[\n  \"a, b: {c}\",\n  \"q\\\"uo\\\\te\",\n  \"\\u65e5\"\n]";
        QTest::newRow("already pretty") << "{\n\t\"a\" :  1 ,\r\n \"b\":-2.5e3\n}"
                                        << "{\n  \"a\": 1,\n  \"b\": -2.5e3\n}";
        QTest::newRow("unicode") << QString::fromUtf8("{\"名前\":\"クリー\"}")
                                 << QString::fromUtf8("{\n  \"名前\": \"クリー\"\n}");
    }
    void json()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QCOMPARE(ResponseFormatter::prettyJson(in), out);
    }

    void invalidJson_data()
    {
        QTest::addColumn<QString>("in");
        QTest::newRow("truncated") << "{\"a\":1";
        QTest::newRow("trailing comma") << "{\"a\":1,}";
        QTest::newRow("not json") << "hello";
        QTest::newRow("empty") << "";
    }
    void invalidJson()
    {
        QFETCH(QString, in);
        QVERIFY(ResponseFormatter::prettyJson(in).isEmpty());
    }

    void xml()
    {
        QCOMPARE(ResponseFormatter::prettyXml("<?xml version=\"1.0\"?><a x=\"1\"><b>t</b><c/>\n  </a>"),
                 QString("<?xml version=\"1.0\"?>\n<a x=\"1\">\n  <b>t</b>\n  <c/>\n</a>\n"));
        QVERIFY(ResponseFormatter::prettyXml("<a><b></a>").isEmpty());
        QVERIFY(ResponseFormatter::prettyXml("plain").isEmpty());
    }

    void formatDetectsKind()
    {
        auto r = ResponseFormatter::format("  {\"a\":1}\n");
        QCOMPARE(r.kind, Kind::Json);
        QCOMPARE(r.text, QString("{\n  \"a\": 1\n}\n"));

        r = ResponseFormatter::format("<r><x/></r>");
        QCOMPARE(r.kind, Kind::Xml);
        QCOMPARE(r.text, QString("<r>\n  <x/>\n</r>\n"));

        // HTML that isn't well-formed XML is left alone.
        const QString html = "<!DOCTYPE html><html><body><p>hi<br></body></html>";
        r = ResponseFormatter::format(html);
        QCOMPARE(r.kind, Kind::None);
        QCOMPARE(r.text, html);

        r = ResponseFormatter::format("plain text");
        QCOMPARE(r.kind, Kind::None);
        QCOMPARE(r.text, QString("plain text"));
    }

    void formatKeepsIncludedHeaders()
    {
        // -i with -L: two header blocks (a redirect, then the final response).
        const QString out =
            "HTTP/1.1 301 Moved\r\nLocation: /b\r\n\r\n"
            "HTTP/2 200\r\ncontent-type: application/json\r\n\r\n"
            "{\"ok\":true}";
        const auto r = ResponseFormatter::format(out);
        QCOMPARE(r.kind, Kind::Json);
        QCOMPARE(r.text, QString("HTTP/1.1 301 Moved\r\nLocation: /b\r\n\r\n"
                                 "HTTP/2 200\r\ncontent-type: application/json\r\n\r\n"
                                 "{\n  \"ok\": true\n}\n"));

        // LF-only line endings, too.
        QCOMPARE(ResponseFormatter::format("HTTP/1.1 200 OK\nA: b\n\n[1]").text,
                 QString("HTTP/1.1 200 OK\nA: b\n\n[\n  1\n]\n"));

        // Headers with no body (e.g. -I) are returned unchanged.
        const QString headOnly = "HTTP/1.1 200 OK\r\nA: b\r\n";
        QCOMPARE(ResponseFormatter::format(headOnly).kind, Kind::None);
        QCOMPARE(ResponseFormatter::format(headOnly).text, headOnly);
    }
};

QTEST_APPLESS_MAIN(TestResponseFormatter)
#include "tst_responseformatter.moc"
