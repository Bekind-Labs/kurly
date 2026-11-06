#include "curlcommand.h"

#include <QRegularExpression>
#include <QStringList>

namespace CurlCommand {

QString shellQuote(const QString &s)
{
    static const QRegularExpression safe(QStringLiteral("^[A-Za-z0-9_@%+=:,./-]+$"));
    if (safe.match(s).hasMatch())
        return s;
    QString q = s;
    q.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + q + QLatin1Char('\'');
}

QString build(const CurlOptions &o)
{
    const QString url = o.url.trimmed();
    if (url.isEmpty())
        return {};

    const auto opt = [&](const char *shortOpt, const char *longOpt) {
        return QString::fromLatin1(o.longOptions ? longOpt : shortOpt);
    };

    // Each entry is one "argument group" (option plus its value) so that
    // multiline output can put each group on its own line.
    QStringList parts;
    parts << QStringLiteral("curl");

    const QString method = o.method.trimmed().toUpper();
    if (method == QLatin1String("HEAD")) {
        parts << opt("-I", "--head");
    } else if (!method.isEmpty() && method != QLatin1String("GET")
               && !(method == QLatin1String("POST") && !o.body.isEmpty())) {
        // curl already switches to POST when a body is supplied.
        parts << opt("-X", "--request") + QLatin1Char(' ') + shellQuote(method);
    }

    if (o.followRedirects) parts << opt("-L", "--location");
    if (o.insecure)        parts << opt("-k", "--insecure");
    if (o.verbose)         parts << opt("-v", "--verbose");
    if (o.silent)          parts << opt("-s", "--silent");
    if (o.includeHeaders)  parts << opt("-i", "--include");
    if (o.failOnError)     parts << opt("-f", "--fail");
    if (o.compressed)      parts << QStringLiteral("--compressed");

    if (o.maxTimeSeconds > 0)
        parts << QStringLiteral("--max-time %1").arg(o.maxTimeSeconds);
    if (!o.user.isEmpty())
        parts << opt("-u", "--user") + QLatin1Char(' ') + shellQuote(o.user);
    if (!o.userAgent.isEmpty())
        parts << opt("-A", "--user-agent") + QLatin1Char(' ') + shellQuote(o.userAgent);

    for (const auto &h : o.headers) {
        const QString name = h.first.trimmed();
        if (name.isEmpty())
            continue;
        parts << opt("-H", "--header") + QLatin1Char(' ')
                     + shellQuote(name + QStringLiteral(": ") + h.second);
    }

    if (!o.body.isEmpty())
        parts << QStringLiteral("--data-raw ") + shellQuote(o.body);
    if (!o.outputFile.isEmpty())
        parts << opt("-o", "--output") + QLatin1Char(' ') + shellQuote(o.outputFile);

    parts << shellQuote(url);

    return parts.join(o.multiline ? QStringLiteral(" \\\n  ") : QStringLiteral(" "));
}

} // namespace CurlCommand
