#pragma once

#include "curlcommand.h"

#include <QStringList>

// Parses an existing curl command line (e.g. from a browser's
// "Copy as cURL") back into CurlOptions. Understands POSIX shell quoting,
// including bash's $'...' strings, and Windows cmd.exe caret escaping.
namespace CurlParser {

enum class Error {
    None,
    Empty,             // nothing but whitespace
    UnterminatedQuote, // a quote was opened but never closed
    NotCurl,           // the command doesn't start with curl
    NoUrl,             // no URL was found
};

struct Result
{
    Error error = Error::None;
    CurlOptions options;
    // Options Kurly can't represent, as written (e.g. "--proxy", "-F").
    // They are skipped; the rest of the command is still imported.
    QStringList ignored;

    bool ok() const { return error == Error::None; }
};

// Split a command line into arguments. Sets *ok to false on an unclosed quote.
QStringList tokenize(const QString &command, bool *ok = nullptr);

Result parse(const QString &command);

} // namespace CurlParser
