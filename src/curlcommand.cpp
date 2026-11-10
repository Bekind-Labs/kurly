#include "curlcommand.h"

#include <QRegularExpression>
#include <QStringList>

bool CurlOptions::operator==(const CurlOptions &o) const
{
    return url == o.url && method == o.method && headers == o.headers && body == o.body
           && user == o.user && userAgent == o.userAgent && outputFile == o.outputFile
           && maxTimeSeconds == o.maxTimeSeconds && followRedirects == o.followRedirects
           && insecure == o.insecure && verbose == o.verbose && silent == o.silent
           && includeHeaders == o.includeHeaders && compressed == o.compressed
           && failOnError == o.failOnError && longOptions == o.longOptions
           && multiline == o.multiline;
}

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

// Each group is one option plus its value(s), unquoted. Keeping groups
// together lets the display put each one on its own line, while
// arguments() can flatten them for running curl directly.
static QList<QStringList> argumentGroups(const CurlOptions &o)
{
    const QString url = o.url.trimmed();
    if (url.isEmpty())
        return {};

    const auto opt = [&](const char *shortOpt, const char *longOpt) {
        return QString::fromLatin1(o.longOptions ? longOpt : shortOpt);
    };

    QList<QStringList> g;

    const QString method = o.method.trimmed().toUpper();
    if (method == QLatin1String("HEAD")) {
        g << QStringList{opt("-I", "--head")};
    } else {
        // curl sends GET by default, or POST when there's a body. Leave -X
        // out only when that default is the chosen method, so the command
        // always sends what the form shows (e.g. GET with a body needs -X GET).
        const QString implied = o.body.isEmpty() ? QStringLiteral("GET") : QStringLiteral("POST");
        if (!method.isEmpty() && method != implied)
            g << QStringList{opt("-X", "--request"), method};
    }

    if (o.followRedirects) g << QStringList{opt("-L", "--location")};
    if (o.insecure)        g << QStringList{opt("-k", "--insecure")};
    if (o.verbose)         g << QStringList{opt("-v", "--verbose")};
    if (o.silent)          g << QStringList{opt("-s", "--silent")};
    if (o.includeHeaders)  g << QStringList{opt("-i", "--include")};
    if (o.failOnError)     g << QStringList{opt("-f", "--fail")};
    if (o.compressed)      g << QStringList{QStringLiteral("--compressed")};

    if (o.maxTimeSeconds > 0)
        g << QStringList{QStringLiteral("--max-time"), QString::number(o.maxTimeSeconds)};
    if (!o.user.isEmpty())
        g << QStringList{opt("-u", "--user"), o.user};
    if (!o.userAgent.isEmpty())
        g << QStringList{opt("-A", "--user-agent"), o.userAgent};

    for (const auto &h : o.headers) {
        const QString name = h.first.trimmed();
        if (!name.isEmpty())
            g << QStringList{opt("-H", "--header"), name + QStringLiteral(": ") + h.second};
    }

    if (!o.body.isEmpty())
        g << QStringList{QStringLiteral("--data-raw"), o.body};
    if (!o.outputFile.isEmpty())
        g << QStringList{opt("-o", "--output"), o.outputFile};

    g << QStringList{url};
    return g;
}

QStringList arguments(const CurlOptions &o)
{
    QStringList args;
    for (const auto &group : argumentGroups(o))
        args += group;
    return args;
}

QString build(const CurlOptions &o)
{
    const auto groups = argumentGroups(o);
    if (groups.isEmpty())
        return {};

    QStringList parts{QStringLiteral("curl")};
    for (const auto &group : groups) {
        QStringList quoted;
        for (const auto &arg : group)
            quoted << shellQuote(arg);
        parts << quoted.join(QLatin1Char(' '));
    }
    return parts.join(o.multiline ? QStringLiteral(" \\\n  ") : QStringLiteral(" "));
}

} // namespace CurlCommand
