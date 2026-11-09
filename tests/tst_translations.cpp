#include <QtTest>

#include <QFile>
#include <QRegularExpression>
#include <QXmlStreamReader>

// Validates the Japanese .ts file directly, so problems show up as test
// failures instead of English strings (or broken %1 placeholders) at runtime.
class TestTranslations : public QObject
{
    Q_OBJECT

    struct Message
    {
        QString context, source, translation, type;
    };
    QList<Message> m_messages;

    static QStringList placeholders(const QString &s)
    {
        static const QRegularExpression re(QStringLiteral("%\\d+"));
        QStringList out;
        for (auto it = re.globalMatch(s); it.hasNext();)
            out << it.next().captured();
        out.sort();
        return out;
    }

private slots:
    void initTestCase()
    {
        QFile f(QStringLiteral(KURLY_TS_FILE));
        QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(f.fileName()));
        QXmlStreamReader xml(&f);
        QString context;
        Message m;
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement())
                continue;
            if (xml.name() == u"TS") {
                QCOMPARE(xml.attributes().value("language").toString(), QString("ja_JP"));
            } else if (xml.name() == u"name") {
                context = xml.readElementText();
            } else if (xml.name() == u"message") {
                m = {context, {}, {}, {}};
            } else if (xml.name() == u"source") {
                m.source = xml.readElementText();
            } else if (xml.name() == u"translation") {
                m.type = xml.attributes().value("type").toString();
                m.translation = xml.readElementText();
                m_messages << m;
            }
        }
        QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
        QVERIFY(m_messages.size() > 50);
    }

    void everyStringIsTranslated()
    {
        for (const auto &m : m_messages) {
            const QString where = m.context + ": " + m.source;
            QVERIFY2(m.type.isEmpty(), qPrintable(where + " is " + m.type));
            QVERIFY2(!m.translation.trimmed().isEmpty(), qPrintable(where + " has no translation"));
        }
    }

    void placeholdersMatch()
    {
        for (const auto &m : m_messages)
            QVERIFY2(placeholders(m.source) == placeholders(m.translation),
                     qPrintable(m.context + ": " + m.source + " → " + m.translation));
    }

    void translationsAreJapanese()
    {
        // Catch copy-pasted English. A few strings are legitimately the same
        // (URLs, "URL:"), so require Japanese characters when the source has
        // more than one word.
        static const QRegularExpression japanese(QStringLiteral("[\\x{3040}-\\x{30ff}\\x{4e00}-\\x{9fff}]"));
        for (const auto &m : m_messages) {
            if (!m.source.contains(' ') || m.source.startsWith("https://"))
                continue;
            QVERIFY2(japanese.match(m.translation).hasMatch(),
                     qPrintable(m.source + " → " + m.translation));
        }
    }

    void noDuplicateSources()
    {
        QSet<QString> seen;
        for (const auto &m : m_messages) {
            const QString key = m.context + '\x1f' + m.source;
            QVERIFY2(!seen.contains(key), qPrintable("duplicate: " + m.source));
            seen.insert(key);
        }
    }
};

QTEST_APPLESS_MAIN(TestTranslations)
#include "tst_translations.moc"
