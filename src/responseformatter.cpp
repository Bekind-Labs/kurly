#include "responseformatter.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace ResponseFormatter {

namespace {
constexpr int kIndent = 2;

void newline(QString &out, int depth)
{
    out += QLatin1Char('\n');
    out += QString(depth * kIndent, QLatin1Char(' '));
}
} // namespace

QString prettyJson(const QString &json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || doc.isNull())
        return {};

    // Valid JSON, so a simple scanner that only tracks strings is enough.
    // (QJsonDocument::toJson would sort object keys alphabetically.)
    QString out;
    out.reserve(json.size() * 2);
    int depth = 0;
    const int n = json.size();
    for (int i = 0; i < n; ++i) {
        const QChar c = json.at(i);
        switch (c.unicode()) {
        case '"': {
            const int start = i++;
            while (i < n && json.at(i) != QLatin1Char('"'))
                i += json.at(i) == QLatin1Char('\\') ? 2 : 1;
            out += QStringView(json).mid(start, i - start + 1);
            break;
        }
        case '{':
        case '[': {
            int j = i + 1;
            while (j < n && json.at(j).isSpace())
                ++j;
            const QChar close = c == QLatin1Char('{') ? QLatin1Char('}') : QLatin1Char(']');
            if (j < n && json.at(j) == close) { // empty container stays on one line
                out += c;
                out += close;
                i = j;
            } else {
                out += c;
                newline(out, ++depth);
            }
            break;
        }
        case '}':
        case ']':
            newline(out, --depth);
            out += c;
            break;
        case ',':
            out += c;
            newline(out, depth);
            break;
        case ':':
            out += QStringLiteral(": ");
            break;
        case ' ': case '\t': case '\n': case '\r':
            break;
        default:
            out += c;
        }
    }
    return out;
}

QString prettyXml(const QString &xml)
{
    QXmlStreamReader reader(xml);
    QString out;
    QXmlStreamWriter writer(&out);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(kIndent);
    bool sawElement = false;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isCharacters() && reader.isWhitespace())
            continue; // re-indented by the writer
        if (reader.isStartDocument() && reader.documentVersion().isEmpty())
            continue; // don't add an <?xml?> declaration that wasn't there
        if (reader.isStartElement())
            sawElement = true;
        if (!reader.hasError())
            writer.writeCurrentToken(reader);
    }
    if (reader.hasError() || !sawElement)
        return {};
    return out.trimmed() + QLatin1Char('\n');
}

Result format(const QString &output)
{
    // Split off any "HTTP/..." header blocks printed by --include.
    QString headers;
    QString body = output;
    while (body.startsWith(QLatin1String("HTTP/"))) {
        int end = body.indexOf(QLatin1String("\r\n\r\n"));
        int sep = 4;
        const int lf = body.indexOf(QLatin1String("\n\n"));
        if (end < 0 || (lf >= 0 && lf < end)) {
            end = lf;
            sep = 2;
        }
        if (end < 0)
            return {Kind::None, output}; // headers only
        headers += body.left(end + sep);
        body = body.mid(end + sep);
    }

    const QString trimmed = body.trimmed();
    if (trimmed.startsWith(QLatin1Char('{')) || trimmed.startsWith(QLatin1Char('['))) {
        const QString pretty = prettyJson(trimmed);
        if (!pretty.isEmpty())
            return {Kind::Json, headers + pretty + QLatin1Char('\n')};
    } else if (trimmed.startsWith(QLatin1Char('<'))) {
        const QString pretty = prettyXml(trimmed);
        if (!pretty.isEmpty())
            return {Kind::Xml, headers + pretty};
    }
    return {Kind::None, output};
}

} // namespace ResponseFormatter
