#include "curlparser.h"

#include <QRegularExpression>
#include <QSet>

#include <cmath>

namespace CurlParser {

namespace {

bool isShellSpace(QChar c)
{
    return c == QLatin1Char(' ') || c == QLatin1Char('\t') || c == QLatin1Char('\n');
}

// bash $'...' escapes.
int decodeAnsiC(const QString &s, int i, QString &out)
{
    // s[i] is the character after a backslash; returns the index after the escape.
    const QChar c = s.at(i);
    switch (c.unicode()) {
    case 'n': out += QLatin1Char('\n'); return i + 1;
    case 't': out += QLatin1Char('\t'); return i + 1;
    case 'r': out += QLatin1Char('\r'); return i + 1;
    case 'a': out += QChar(0x07); return i + 1;
    case 'b': out += QChar(0x08); return i + 1;
    case 'e': case 'E': out += QChar(0x1b); return i + 1;
    case 'f': out += QChar(0x0c); return i + 1;
    case 'v': out += QChar(0x0b); return i + 1;
    case 'x': case 'u': case 'U': {
        const int maxDigits = c == QLatin1Char('x') ? 2 : c == QLatin1Char('u') ? 4 : 8;
        int j = i + 1;
        QString digits;
        while (j < s.size() && digits.size() < maxDigits && isxdigit(s.at(j).toLatin1()))
            digits += s.at(j++);
        if (digits.isEmpty()) {
            out += QLatin1Char('\\');
            out += c;
            return i + 1;
        }
        const char32_t code = digits.toUInt(nullptr, 16);
        out += QString::fromUcs4(&code, 1);
        return j;
    }
    default:
        if (c >= QLatin1Char('0') && c <= QLatin1Char('7')) {
            int j = i, value = 0;
            while (j < s.size() && j < i + 3 && s.at(j) >= QLatin1Char('0') && s.at(j) <= QLatin1Char('7'))
                value = value * 8 + (s.at(j++).unicode() - '0');
            out += QChar(value);
            return j;
        }
        // \\ \' \" \? and anything unknown: the character itself.
        if (c == QLatin1Char('\\') || c == QLatin1Char('\'') || c == QLatin1Char('"')
            || c == QLatin1Char('?')) {
            out += c;
        } else {
            out += QLatin1Char('\\');
            out += c;
        }
        return i + 1;
    }
}

QStringList tokenizePosix(const QString &s, bool *ok)
{
    QStringList args;
    QString cur;
    bool inWord = false;
    int i = 0;
    const int n = s.size();

    while (i < n) {
        const QChar c = s.at(i);
        if (isShellSpace(c)) {
            if (inWord) {
                args << cur;
                cur.clear();
                inWord = false;
            }
            ++i;
        } else if (c == QLatin1Char('\\')) {
            if (i + 1 < n) {
                if (s.at(i + 1) != QLatin1Char('\n')) { // backslash-newline = continuation
                    cur += s.at(i + 1);
                    inWord = true;
                }
                i += 2;
            } else {
                ++i;
            }
        } else if (c == QLatin1Char('\'')) {
            const int end = s.indexOf(QLatin1Char('\''), i + 1);
            if (end < 0) { *ok = false; return args; }
            cur += s.mid(i + 1, end - i - 1);
            inWord = true;
            i = end + 1;
        } else if (c == QLatin1Char('$') && i + 1 < n && s.at(i + 1) == QLatin1Char('\'')) {
            i += 2;
            bool closed = false;
            while (i < n) {
                const QChar d = s.at(i);
                if (d == QLatin1Char('\'')) { closed = true; ++i; break; }
                if (d == QLatin1Char('\\') && i + 1 < n)
                    i = decodeAnsiC(s, i + 1, cur);
                else { cur += d; ++i; }
            }
            if (!closed) { *ok = false; return args; }
            inWord = true;
        } else if (c == QLatin1Char('"')) {
            ++i;
            bool closed = false;
            while (i < n) {
                const QChar d = s.at(i);
                if (d == QLatin1Char('"')) { closed = true; ++i; break; }
                if (d == QLatin1Char('\\') && i + 1 < n) {
                    const QChar e = s.at(i + 1);
                    if (e == QLatin1Char('\n')) { i += 2; continue; }
                    if (QStringLiteral("$`\"\\").contains(e)) { cur += e; i += 2; continue; }
                }
                cur += d;
                ++i;
            }
            if (!closed) { *ok = false; return args; }
            inWord = true;
        } else if (!inWord && c == QLatin1Char('#')) {
            // Comment: skip to end of line.
            while (i < n && s.at(i) != QLatin1Char('\n'))
                ++i;
        } else if (QStringLiteral("|;&<>").contains(c)) {
            // Start of a pipe, redirect or next command: stop here.
            break;
        } else {
            cur += c;
            inWord = true;
            ++i;
        }
    }
    if (inWord)
        args << cur;
    return args;
}

// cmd.exe pass: strip caret escapes and line continuations. Quotes are kept
// for the C runtime argument parser that runs afterwards, as on Windows.
QString unescapeCmd(const QString &s, bool *ok)
{
    QString out;
    bool quoted = false;
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s.at(i);
        if (c == QLatin1Char('"')) {
            quoted = !quoted;
            out += c;
        } else if (c == QLatin1Char('^') && !quoted && i + 1 < s.size()) {
            const QChar next = s.at(i + 1);
            if (next == QLatin1Char('\n')) {
                // "^<newline><newline>" is an escaped (literal) newline;
                // "^<newline>" alone is a line continuation.
                if (i + 2 < s.size() && s.at(i + 2) == QLatin1Char('\n')) {
                    out += QLatin1Char('\n');
                    i += 2;
                } else {
                    ++i;
                }
            } else {
                out += next;
                ++i;
            }
        } else {
            out += c;
        }
    }
    *ok = !quoted;
    return out;
}

// Microsoft C runtime argument splitting.
QStringList tokenizeMsvcrt(const QString &s, bool *ok)
{
    QStringList args;
    QString cur;
    bool inWord = false, quoted = false;
    for (int i = 0; i < s.size();) {
        const QChar c = s.at(i);
        if (c == QLatin1Char('\\')) {
            int j = i;
            while (j < s.size() && s.at(j) == QLatin1Char('\\'))
                ++j;
            const int count = j - i;
            if (j < s.size() && s.at(j) == QLatin1Char('"')) {
                cur += QString(count / 2, QLatin1Char('\\'));
                if (count % 2) { cur += QLatin1Char('"'); i = j + 1; }
                else i = j; // the quote toggles quoting below
            } else {
                cur += QString(count, QLatin1Char('\\'));
                i = j;
            }
            inWord = true;
        } else if (c == QLatin1Char('"')) {
            quoted = !quoted;
            inWord = true;
            ++i;
        } else if (!quoted && isShellSpace(c)) {
            if (inWord) { args << cur; cur.clear(); inWord = false; }
            ++i;
        } else {
            cur += c;
            inWord = true;
            ++i;
        }
    }
    if (quoted)
        *ok = false;
    if (inWord)
        args << cur;
    return args;
}

bool looksLikeCmd(const QString &s)
{
    static const QRegularExpression re(QStringLiteral("\\^\\n|\\^\""));
    return re.match(s).hasMatch();
}

} // namespace

QStringList tokenize(const QString &command, bool *ok)
{
    bool good = true;
    QString s = command;
    s.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    QStringList args;
    if (looksLikeCmd(s)) {
        const QString stage1 = unescapeCmd(s, &good);
        args = tokenizeMsvcrt(stage1, &good);
    } else {
        args = tokenizePosix(s, &good);
    }
    if (ok)
        *ok = good;
    return args;
}

Result parse(const QString &command)
{
    Result r;
    if (command.trimmed().isEmpty()) {
        r.error = Error::Empty;
        return r;
    }

    bool ok = true;
    QStringList args = tokenize(command, &ok);
    if (!ok) {
        r.error = Error::UnterminatedQuote;
        return r;
    }
    if (!args.isEmpty() && args.first() == QLatin1String("$")) // pasted shell prompt
        args.removeFirst();
    if (args.isEmpty()) {
        r.error = Error::Empty;
        return r;
    }
    const QString program = args.first().section(QRegularExpression(QStringLiteral("[/\\\\]")), -1);
    if (program.compare(QLatin1String("curl"), Qt::CaseInsensitive) != 0
        && program.compare(QLatin1String("curl.exe"), Qt::CaseInsensitive) != 0) {
        r.error = Error::NotCurl;
        return r;
    }

    CurlOptions &o = r.options;
    o.method.clear();
    QString explicitMethod;
    bool head = false, json = false;
    QStringList data;

    const auto addHeader = [&](const QString &h) {
        const int colon = h.indexOf(QLatin1Char(':'));
        if (colon > 0) {
            o.headers.append({h.left(colon).trimmed(), h.mid(colon + 1).trimmed()});
        } else if (h.endsWith(QLatin1Char(';'))) {
            o.headers.append({h.chopped(1).trimmed(), QString()});
        } else if (!h.trimmed().isEmpty()) {
            r.ignored << QStringLiteral("-H ") + h;
        }
    };
    const auto addUrl = [&](const QString &u) {
        if (o.url.isEmpty()) o.url = u;
        else r.ignored << u;
    };

    // Options that take a value. Supported ones are applied; the rest are
    // skipped together with their value so it isn't mistaken for the URL.
    static const QSet<QString> unsupportedWithValue = {
        "-x", "--proxy", "-U", "--proxy-user", "-w", "--write-out", "-F", "--form",
        "--form-string", "-T", "--upload-file", "-c", "--cookie-jar", "-r", "--range",
        "--connect-timeout", "--retry", "--retry-delay", "--retry-max-time", "--cacert",
        "--capath", "-E", "--cert", "--key", "--cert-type", "--key-type", "--resolve",
        "--connect-to", "--interface", "--limit-rate", "-K", "--config", "--max-redirs",
        "-y", "--speed-time", "-Y", "--speed-limit", "-z", "--time-cond", "-t",
        "--telnet-option", "-Q", "--quote", "--oauth2-bearer", "--unix-socket",
        "--abstract-unix-socket", "--aws-sigv4", "-D", "--dump-header", "--trace",
        "--trace-ascii", "--stderr", "--local-port", "--dns-servers", "-P", "--ftp-port",
        "--variable", "--expand-url", "--url-query", "-C", "--continue-at", "--noproxy",
        "--proxy-header", "--pinnedpubkey", "--ciphers", "--tls-max", "--happy-eyeballs-timeout-ms",
        "--keepalive-time", "--output-dir", "--create-file-mode", "--etag-save",
        "--etag-compare", "--hsts", "--alt-svc", "--sasl-authzid", "--service-name",
        "--login-options", "--mail-from", "--mail-rcpt", "--mail-auth",
    };
    // Flags that don't change the request Kurly builds; dropped silently.
    static const QSet<QString> harmless = {
        "-S", "--show-error", "--no-progress-meter", "-#", "--progress-bar",
        "-N", "--no-buffer", "--no-keepalive", "-n", "--netrc", "--http1.1",
        "--http2", "--http2-prior-knowledge", "-0", "--http1.0", "--globoff", "-g",
        "--path-as-is", "-O", "--remote-name",
    };

    // Returns true if `opt` consumed `value`.
    const auto apply = [&](const QString &opt, const QString *value) -> bool {
        const auto need = [&]() { return value != nullptr; };
        if (opt == "-L" || opt == "--location" || opt == "--location-trusted") o.followRedirects = true;
        else if (opt == "-k" || opt == "--insecure") o.insecure = true;
        else if (opt == "-v" || opt == "--verbose") o.verbose = true;
        else if (opt == "-s" || opt == "--silent") o.silent = true;
        else if (opt == "-i" || opt == "--include" || opt == "--show-headers") o.includeHeaders = true;
        else if (opt == "-f" || opt == "--fail" || opt == "--fail-with-body") o.failOnError = true;
        else if (opt == "--compressed") o.compressed = true;
        else if (opt == "-I" || opt == "--head") head = true;
        else if (opt == "-G" || opt == "--get") r.ignored << opt;
        else if (harmless.contains(opt)) {}
        else if (!need()) return false; // caller will supply the value
        else if (opt == "-X" || opt == "--request") { explicitMethod = value->toUpper(); return true; }
        else if (opt == "-H" || opt == "--header") {
            if (value->startsWith(QLatin1Char('@'))) r.ignored << opt + QLatin1Char(' ') + *value;
            else addHeader(*value);
            return true;
        }
        else if (opt == "-d" || opt == "--data" || opt == "--data-ascii" || opt == "--data-binary") {
            if (value->startsWith(QLatin1Char('@'))) r.ignored << opt + QLatin1Char(' ') + *value;
            else data << *value;
            return true;
        }
        else if (opt == "--data-raw" || opt == "--data-urlencode") { data << *value; return true; }
        else if (opt == "--json") { data << *value; json = true; return true; }
        else if (opt == "-u" || opt == "--user") { o.user = *value; return true; }
        else if (opt == "-A" || opt == "--user-agent") { o.userAgent = *value; return true; }
        else if (opt == "-e" || opt == "--referer") { o.headers.append({QStringLiteral("Referer"), *value}); return true; }
        else if (opt == "-b" || opt == "--cookie") {
            if (CurlCommand::splitCookies(*value, &o.cookies)) {}
            else if (value->contains(QLatin1Char('='))) o.headers.append({QStringLiteral("Cookie"), *value});
            else r.ignored << opt + QLatin1Char(' ') + *value; // a cookie file
            return true;
        }
        else if (opt == "-o" || opt == "--output") { o.outputFile = *value; return true; }
        else if (opt == "-m" || opt == "--max-time") {
            bool okNum = false;
            const double secs = value->toDouble(&okNum);
            if (okNum && secs > 0) o.maxTimeSeconds = int(std::ceil(secs));
            return true;
        }
        else if (opt == "--url") { addUrl(*value); return true; }
        else if (unsupportedWithValue.contains(opt)) { r.ignored << opt; return true; }
        return true;
    };

    static const QString shortWithValue = QStringLiteral("XHduAebomxUwFTcrEKyYztQDPC");

    bool endOfOptions = false;
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args.at(i);
        if (endOfOptions || !a.startsWith(QLatin1Char('-')) || a == QLatin1String("-")) {
            addUrl(a);
            continue;
        }
        if (a == QLatin1String("--")) {
            endOfOptions = true;
            continue;
        }
        if (a.startsWith(QLatin1String("--"))) {
            if (!apply(a, nullptr)) {
                const bool known = a == "--request" || a == "--header" || a.startsWith("--data")
                                   || a == "--json" || a == "--user" || a == "--user-agent"
                                   || a == "--referer" || a == "--cookie" || a == "--output"
                                   || a == "--max-time" || a == "--url"
                                   || unsupportedWithValue.contains(a);
                if (known && i + 1 < args.size()) {
                    apply(a, &args.at(++i));
                } else {
                    r.ignored << a;
                }
            }
            continue;
        }
        // Short options, possibly clustered (-sSL) or with an attached value (-XPOST).
        for (int k = 1; k < a.size(); ++k) {
            const QString opt = QStringLiteral("-") + a.at(k);
            if (shortWithValue.contains(a.at(k))) {
                QString value = a.mid(k + 1);
                if (value.isEmpty() && i + 1 < args.size())
                    value = args.at(++i);
                apply(opt, &value);
                break;
            }
            if (!apply(opt, nullptr))
                r.ignored << opt;
        }
    }

    if (o.url.isEmpty()) {
        r.error = Error::NoUrl;
        return r;
    }

    if (!data.isEmpty())
        o.body = data.join(json ? QString() : QStringLiteral("&"));
    if (json) {
        const auto has = [&](const char *name) {
            for (const auto &h : o.headers)
                if (h.first.compare(QLatin1String(name), Qt::CaseInsensitive) == 0) return true;
            return false;
        };
        if (!has("Content-Type")) o.headers.append({QStringLiteral("Content-Type"), QStringLiteral("application/json")});
        if (!has("Accept")) o.headers.append({QStringLiteral("Accept"), QStringLiteral("application/json")});
    }

    // Browsers send cookies as a Cookie header; edit them as cookies instead.
    for (qsizetype i = 0; i < o.headers.size();) {
        if (o.headers.at(i).first.trimmed().compare(QLatin1String("Cookie"), Qt::CaseInsensitive) == 0
            && CurlCommand::splitCookies(o.headers.at(i).second, &o.cookies))
            o.headers.removeAt(i);
        else
            ++i;
    }

    if (!explicitMethod.isEmpty()) o.method = explicitMethod;
    else if (head) o.method = QStringLiteral("HEAD");
    else if (!o.body.isEmpty()) o.method = QStringLiteral("POST");
    else o.method = QStringLiteral("GET");

    return r;
}

} // namespace CurlParser
