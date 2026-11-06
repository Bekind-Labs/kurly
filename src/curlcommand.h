#pragma once

#include <QList>
#include <QPair>
#include <QString>

// A description of a curl invocation, and the logic that turns it into a
// shell-ready command line. Has no GUI dependencies so it can be unit tested.
struct CurlOptions
{
    QString url;
    QString method = QStringLiteral("GET");
    QList<QPair<QString, QString>> headers; // only non-empty, enabled headers
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
};

namespace CurlCommand {

// Quote a string as a single POSIX shell word. Words made only of safe
// characters are returned unquoted.
QString shellQuote(const QString &s);

// Build the command line. Returns an empty string if there is no URL yet.
QString build(const CurlOptions &o);

} // namespace CurlCommand
