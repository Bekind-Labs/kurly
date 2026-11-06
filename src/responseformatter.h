#pragma once

#include <QString>

// Pretty-prints response bodies for display. GUI-independent.
namespace ResponseFormatter {

enum class Kind { None, Json, Xml };

struct Result
{
    Kind kind = Kind::None; // None: text returned unchanged
    QString text;
};

// Re-indents JSON, keeping key order and the original string escapes.
// Returns an empty string if `json` isn't a valid JSON object or array.
QString prettyJson(const QString &json);

// Re-indents well-formed XML. Returns an empty string if it isn't.
QString prettyXml(const QString &xml);

// Formats curl's stdout. Leading HTTP header blocks (from -i/--include,
// possibly several when following redirects) are kept as-is above the
// formatted body.
Result format(const QString &output);

} // namespace ResponseFormatter
