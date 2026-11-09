#include <QtTest>

#include "curlparser.h"

using Headers = QList<QPair<QString, QString>>;

class TestCurlParser : public QObject
{
    Q_OBJECT
private slots:
    void chromeBash()
    {
        // Shape of Chrome DevTools "Copy as cURL (bash)".
        const QString cmd = QString::fromUtf8(
            "curl 'https://api.example.com/v1/items?page=2' \\\n"
            "  -H 'accept: application/json, text/plain, */*' \\\n"
            "  -H 'accept-language: en-US,en;q=0.9,ja;q=0.8' \\\n"
            "  -H 'content-type: application/json' \\\n"
            "  -b 'session=abc123; theme=dark' \\\n"
            "  -H 'user-agent: Mozilla/5.0 (Macintosh)' \\\n"
            "  --data-raw $'{\"name\":\"it\\'s\",\"note\":\"line1\\\\nline2\",\"jp\":\"\\u65e5\"}'\n");
        const auto r = CurlParser::parse(cmd);
        QVERIFY(r.ok());
        QCOMPARE(r.options.url, QString("https://api.example.com/v1/items?page=2"));
        QCOMPARE(r.options.method, QString("POST"));
        QCOMPARE(r.options.headers,
                 (Headers{{"accept", "application/json, text/plain, */*"},
                          {"accept-language", "en-US,en;q=0.9,ja;q=0.8"},
                          {"content-type", "application/json"},
                          {"Cookie", "session=abc123; theme=dark"},
                          {"user-agent", "Mozilla/5.0 (Macintosh)"}}));
        QCOMPARE(r.options.body,
                 QString::fromUtf8("{\"name\":\"it's\",\"note\":\"line1\\nline2\",\"jp\":\"日\"}"));
        QVERIFY(r.ignored.isEmpty());
    }

    void chromeCmd()
    {
        // Shape of Chrome DevTools "Copy as cURL (cmd)": ^" quoting, carets,
        // \" for the C runtime, and ^<newline><newline> for a literal newline.
        const QString cmd =
            "curl ^\"https://api.example.com/v1/items?a=1^&b=2^\" ^\r\n"
            "  -H ^\"accept: application/json^\" ^\r\n"
            "  -H ^\"content-type: application/json^\" ^\r\n"
            "  --data-raw ^\"^{^\\^\"name^\\^\":^\\^\"kurly^\\^\",^\\^\"p^\\^\":^\\^\"50^%^\\^\"^}^\" ^\r\n"
            "  --compressed";
        const auto r = CurlParser::parse(cmd);
        QVERIFY(r.ok());
        QCOMPARE(r.options.url, QString("https://api.example.com/v1/items?a=1&b=2"));
        QCOMPARE(r.options.headers,
                 (Headers{{"accept", "application/json"}, {"content-type", "application/json"}}));
        QCOMPARE(r.options.body, QString("{\"name\":\"kurly\",\"p\":\"50%\"}"));
        QVERIFY(r.options.compressed);
        QCOMPARE(r.options.method, QString("POST"));
    }

    void oldStyleCmd()
    {
        const auto r = CurlParser::parse(
            "curl \"https://example.com/x\" -H \"X-A: 1\" --data-binary \"{\\\"a\\\":1}\"");
        QVERIFY(r.ok());
        QCOMPARE(r.options.url, QString("https://example.com/x"));
        QCOMPARE(r.options.body, QString("{\"a\":1}"));
    }

    void firefox()
    {
        const auto r = CurlParser::parse(
            "curl 'https://example.com/api' -X 'PUT' -H 'Accept: */*' "
            "-H 'Content-Type: text/plain;charset=UTF-8' --data-raw 'hello'");
        QVERIFY(r.ok());
        QCOMPARE(r.options.method, QString("PUT"));
        QCOMPARE(r.options.body, QString("hello"));
    }

    void clusteredAndAttached()
    {
        const auto r = CurlParser::parse(
            "curl -sSLk -XDELETE -Hx-a:1 -m 2.5 -u me:pw -A ua -e https://ref/ "
            "-o out.json -v -i -f https://example.com");
        QVERIFY(r.ok());
        QVERIFY(r.options.silent && r.options.followRedirects && r.options.insecure);
        QVERIFY(r.options.verbose && r.options.includeHeaders && r.options.failOnError);
        QCOMPARE(r.options.method, QString("DELETE"));
        QCOMPARE(r.options.maxTimeSeconds, 3);
        QCOMPARE(r.options.user, QString("me:pw"));
        QCOMPARE(r.options.userAgent, QString("ua"));
        QCOMPARE(r.options.outputFile, QString("out.json"));
        QCOMPARE(r.options.headers, (Headers{{"x-a", "1"}, {"Referer", "https://ref/"}}));
        QVERIFY(r.ignored.isEmpty());
    }

    void headAndDataJoin()
    {
        auto r = CurlParser::parse("curl -I https://example.com");
        QCOMPARE(r.options.method, QString("HEAD"));
        r = CurlParser::parse("curl -d a=1 -d b=2 https://example.com");
        QCOMPARE(r.options.body, QString("a=1&b=2"));
        QCOMPARE(r.options.method, QString("POST"));
    }

    void json()
    {
        const auto r = CurlParser::parse("curl --json '{\"a\":1}' https://example.com");
        QVERIFY(r.ok());
        QCOMPARE(r.options.method, QString("POST"));
        QCOMPARE(r.options.body, QString("{\"a\":1}"));
        QCOMPARE(r.options.headers, (Headers{{"Content-Type", "application/json"},
                                              {"Accept", "application/json"}}));
    }

    void unsupportedAreReported()
    {
        const auto r = CurlParser::parse(
            "curl --proxy http://p:8080 -F file=@a.txt --http2 -S --weird "
            "https://example.com | jq .");
        QVERIFY(r.ok());
        QCOMPARE(r.options.url, QString("https://example.com"));
        QCOMPARE(r.ignored, (QStringList{"--proxy", "-F", "--weird"}));
    }

    void doubleQuotesAndPrompt()
    {
        const auto r = CurlParser::parse("$ curl \"https://example.com/?q=$HOME\\\"x\" -H \"A: b c\"");
        QVERIFY(r.ok());
        QCOMPARE(r.options.url, QString("https://example.com/?q=$HOME\"x"));
    }

    void errors()
    {
        QCOMPARE(CurlParser::parse("   ").error, CurlParser::Error::Empty);
        QCOMPARE(CurlParser::parse("curl 'https://x").error, CurlParser::Error::UnterminatedQuote);
        QCOMPARE(CurlParser::parse("wget https://x").error, CurlParser::Error::NotCurl);
        QCOMPARE(CurlParser::parse("curl -L -H 'a: b'").error, CurlParser::Error::NoUrl);
        QVERIFY(CurlParser::parse("/usr/bin/curl https://x").ok());
        QVERIFY(CurlParser::parse("curl.exe https://x").ok());
    }

    void ansiCEscapes()
    {
        bool ok = true;
        const auto args = CurlParser::tokenize(
            "curl $'a\\tb\\rc\\ad\\be\\ef\\fg\\vh' $'\\x41\\x\\101\\q\\?\\\"\\\\' $'\\U0001F600'", &ok);
        QVERIFY(ok);
        QCOMPARE(args.size(), 4);
        QCOMPARE(args[1], QString("a\tb\rc\ad\be\x1b" "f\fg\vh"));
        // \x41, \x with no digits (kept), octal 101, unknown \q (kept), \? \" \\.
        QCOMPARE(args[2], QString("A\\xA\\q?\"\\"));
        QCOMPARE(args[3], QString::fromUtf8("\xF0\x9F\x98\x80"));
        CurlParser::tokenize("curl $'open", &ok);
        QVERIFY(!ok);
        CurlParser::tokenize("curl \"open", &ok);
        QVERIFY(!ok);
    }

    void posixDetails()
    {
        bool ok = true;
        // Trailing lone backslash, comments, empty quoted args, adjacent quoting.
        QCOMPARE(CurlParser::tokenize("curl a'b'\"c\"d '' # comment here\n -L \\", &ok),
                 (QStringList{"curl", "abcd", "", "-L"}));
        QVERIFY(ok);
        QCOMPARE(CurlParser::tokenize("curl \"a\\nb\\\nc\"", &ok), (QStringList{"curl", "a\\nbc"}));
    }

    void windowsDetails()
    {
        bool ok = true;
        // Plain-quoted cmd style: carets inside quotes are literal; even
        // backslashes before a quote halve and the quote still closes;
        // backslashes elsewhere are literal.
        QCOMPARE(CurlParser::tokenize("curl \"a^b\" ^\n \"c\\\\\" d\\e", &ok),
                 (QStringList{"curl", "a^b", "c\\", "d\\e"}));
        QVERIFY(ok);
        // Escaped literal newline (^ + two newlines) inside a ^"-quoted arg.
        QCOMPARE(CurlParser::tokenize("curl ^\"line1^\n\nline2^\"", &ok),
                 (QStringList{"curl", "line1\nline2"}));
        CurlParser::tokenize("curl ^\"open \"x", &ok);
        QVERIFY(!ok);
    }

    void optionDetails()
    {
        const auto r = CurlParser::parse(
            "curl -H 'X-Empty;' -H 'nocolon' -H @headers.txt -b cookies.txt -d @body.json "
            "--data-urlencode q=a+b -G -m soon -q --url https://first.test https://second.test "
            "-- -not-an-option");
        QVERIFY(r.ok());
        QCOMPARE(r.options.url, QString("https://first.test"));
        QCOMPARE(r.options.headers, (Headers{{"X-Empty", ""}}));
        QCOMPARE(r.options.body, QString("q=a+b"));
        QCOMPARE(r.options.maxTimeSeconds, 0);
        QCOMPARE(r.ignored, (QStringList{"-H nocolon", "-H @headers.txt", "-b cookies.txt",
                                         "-d @body.json", "-G", "-q", "https://second.test",
                                         "-not-an-option"}));
    }

    void onlyPrompt()
    {
        QCOMPARE(CurlParser::parse("$").error, CurlParser::Error::Empty);
        QCOMPARE(CurlParser::parse("# just a comment").error, CurlParser::Error::Empty);
    }

    void roundTrip()
    {
        CurlOptions o;
        o.url = "https://example.com/?a=1&b=it's";
        o.method = "PATCH";
        o.headers = {{"X-One", "1"}, {"X-Two", "two words"}};
        o.body = "{\"a\": \"multi\nline 'quoted'\"}";
        o.user = "u:p";
        o.followRedirects = o.compressed = o.failOnError = true;
        o.maxTimeSeconds = 7;
        for (bool longOpts : {false, true}) {
            for (bool multiline : {false, true}) {
                o.longOptions = longOpts;
                o.multiline = multiline;
                const auto r = CurlParser::parse(CurlCommand::build(o));
                QVERIFY(r.ok());
                QCOMPARE(r.options.url, o.url);
                QCOMPARE(r.options.method, o.method);
                QCOMPARE(r.options.headers, o.headers);
                QCOMPARE(r.options.body, o.body);
                QCOMPARE(r.options.user, o.user);
                QCOMPARE(r.options.maxTimeSeconds, o.maxTimeSeconds);
                QVERIFY(r.options.followRedirects && r.options.compressed && r.options.failOnError);
            }
        }
    }
};

QTEST_APPLESS_MAIN(TestCurlParser)
#include "tst_curlparser.moc"
