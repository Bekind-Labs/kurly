#pragma once

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

// A description of a curl invocation, and the logic that turns it into a
// shell-ready command line. Has no GUI dependencies so it can be unit tested.
struct CurlOptions
{
    QString url;
    QString method = QStringLiteral("GET");
    QList<QPair<QString, QString>> headers; // only non-empty, enabled headers
    QList<QPair<QString, QString>> cookies; // name/value pairs, sent with -b
    QString body;
    QString user;     // "user" or "user:password"
    QString userAgent;
    QString outputFile;
    int maxTimeSeconds = 0; // 0 = unset

    bool followRedirects = false;
    bool insecure = false;
    bool verbose = false;
    bool silent = false;
    bool includeHeaders = false;
    bool compressed = false;
    bool failOnError = false;

    bool longOptions = false; // --location instead of -L
    bool multiline = false;   // one argument per line, joined with backslashes

    bool operator==(const CurlOptions &other) const;
    bool operator!=(const CurlOptions &other) const { return !(*this == other); }
};

namespace CurlCommand {

// Quote a string as a single POSIX shell word. Words made only of safe
// characters are returned unquoted.
QString shellQuote(const QString &s);

// "a=1; b=2", the form curl's -b and the Cookie header take. Cookies without
// a name are left out.
QString cookieString(const QList<QPair<QString, QString>> &cookies);

// The reverse: splits "a=1; b=2" into name/value pairs. Returns false (and
// leaves *out alone) if a part isn't name=value, e.g. a cookie file name.
bool splitCookies(const QString &s, QList<QPair<QString, QString>> *out);

// The arguments to pass to the curl executable (not including "curl"
// itself), unquoted. Empty if there is no URL yet.
QStringList arguments(const CurlOptions &o);

// Build the command line for display/copying. Returns an empty string if there is no URL yet.
QString build(const CurlOptions &o);

} // namespace CurlCommand
