#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "requeststore.h"

class TestRequestStore : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    QString path() const { return m_dir.filePath("sub/library.json"); }

    static CurlOptions sample()
    {
        CurlOptions o;
        o.url = "https://api.example.com/v1/users?x=1";
        o.method = "POST";
        o.headers = {{"Accept", "application/json"}, {"X-名前", "値"}};
        o.body = "{\"a\": 1}\nline two";
        o.user = "me:pw";
        o.userAgent = "UA";
        o.outputFile = "out.json";
        o.maxTimeSeconds = 12;
        o.followRedirects = o.compressed = o.failOnError = true;
        return o;
    }

    void writeFile(const QByteArray &data)
    {
        QDir().mkpath(QFileInfo(path()).absolutePath());
        QFile f(path());
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(data);
    }

private slots:
    void init()
    {
        QDir(m_dir.path()).removeRecursively();
        QDir().mkpath(m_dir.path());
    }

    void newLibraryHasOneEmptyRequest()
    {
        RequestStore store(path());
        QCOMPARE(store.load(), RequestStore::LoadResult::New);
        QCOMPARE(store.count(), 1);
        QCOMPARE(store.currentId(), store.requests().first().id);
        QVERIFY(store.requests().first().options.url.isEmpty());
        QVERIFY(!store.isReadOnly());
        QVERIFY(!QFile::exists(path())); // nothing written until something changes
    }

    void saveAndReload()
    {
        QString id1, id2;
        {
            RequestStore store(path());
            store.load();
            id1 = store.currentId();
            store.setOptions(id1, sample());
            id2 = store.add(CurlOptions{}, "  Second  ");
            store.setCurrentId(id2);
            QVERIFY(store.save());
        }
        RequestStore again(path());
        QCOMPARE(again.load(), RequestStore::LoadResult::Loaded);
        QCOMPARE(again.count(), 2);
        QCOMPARE(again.requests()[0].id, id1);
        QCOMPARE(again.requests()[0].options, sample());
        QCOMPARE(again.requests()[1].name, QString("Second")); // trimmed
        QCOMPARE(again.currentId(), id2);
    }

    void fileIsPrivateAndReadable()
    {
        RequestStore store(path());
        store.load();
        store.setOptions(store.currentId(), sample());
        QVERIFY(store.save());
#ifndef Q_OS_WIN
        QCOMPARE(QFile::permissions(path()) & (QFile::ReadGroup | QFile::ReadOther | QFile::WriteGroup | QFile::WriteOther),
                 QFileDevice::Permissions());
#endif
        QFile f(path());
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        QCOMPARE(root.value("version").toInt(), RequestStore::kFormatVersion);
        const QJsonObject r = root.value("requests").toArray().first().toObject();
        QCOMPARE(r.value("method").toString(), QString("POST"));
        QCOMPARE(r.value("headers").toArray().size(), 2);
        QCOMPARE(r.value("options").toObject().value("followRedirects").toBool(), true);
    }

    void autosaveAndFlush()
    {
        RequestStore store(path());
        store.setSaveDelay(50);
        store.load();
        store.setOptions(store.currentId(), sample());
        QVERIFY(!QFile::exists(path()));
        QTRY_VERIFY(QFile::exists(path())); // saved after the delay

        store.setSaveDelay(60000);
        store.rename(store.currentId(), "Renamed");
        store.flush(); // saves without waiting
        RequestStore check(path());
        check.load();
        QCOMPARE(check.requests().first().name, QString("Renamed"));
    }

    void destructorSavesPendingChanges()
    {
        {
            RequestStore store(path());
            store.setSaveDelay(60000);
            store.load();
            store.add(sample(), "Pending");
        }
        RequestStore check(path());
        check.load();
        QCOMPARE(check.count(), 2);
    }

    void displayPreferencesAreNotStored()
    {
        RequestStore store(path());
        store.load();
        CurlOptions o = sample();
        o.longOptions = o.multiline = true;
        store.setOptions(store.currentId(), o);
        QVERIFY(!store.requests().first().options.longOptions);
        QVERIFY(!store.requests().first().options.multiline);
    }

    void editing()
    {
        RequestStore store(path());
        store.load();
        QSignalSpy list(&store, &RequestStore::listChanged);
        QSignalSpy current(&store, &RequestStore::currentChanged);
        QSignalSpy request(&store, &RequestStore::requestChanged);

        const QString a = store.currentId();
        store.setOptions(a, sample());
        QCOMPARE(request.count(), 1);
        store.setOptions(a, sample()); // unchanged: no signal
        QCOMPARE(request.count(), 1);

        const QString b = store.add({}, "B");
        const QString c = store.add({}, "C");
        const QString a2 = store.duplicate(a); // right after the original
        QCOMPARE(store.indexOf(a2), 1);
        QCOMPARE(store.find(a2)->options, sample());
        QVERIFY(a2 != a);

        store.move(3, 0); // C to the top
        QCOMPARE(store.requests()[0].id, c);
        store.move(0, 99); // out of range: ignored
        QCOMPARE(store.requests()[0].id, c);

        store.setCurrentId(b);
        QCOMPARE(current.count(), 1);
        store.setCurrentId("no-such-id");
        QCOMPARE(store.currentId(), b);

        // Removing the current request selects its neighbour.
        const int bIndex = store.indexOf(b);
        store.remove(b);
        QCOMPARE(store.count(), 3);
        QCOMPARE(store.indexOf(store.currentId()), qMin(bIndex, store.count() - 1));
        QCOMPARE(current.count(), 2);
        QVERIFY(list.count() >= 5);
    }

    void unknownIdsAreIgnored()
    {
        RequestStore store(path());
        store.load();
        QSignalSpy any(&store, &RequestStore::requestChanged);
        QVERIFY(store.duplicate("nope").isEmpty());
        store.remove("nope");
        store.rename("nope", "x");
        store.setOptions("nope", sample());
        QCOMPARE(store.count(), 1);
        QCOMPARE(any.count(), 0);
        QVERIFY(!store.find("nope"));
    }

    void removingLastLeavesEmptyRequest()
    {
        RequestStore store(path());
        store.load();
        const QString only = store.currentId();
        store.setOptions(only, sample());
        store.remove(only);
        QCOMPARE(store.count(), 1);
        QVERIFY(store.currentId() != only);
        QVERIFY(store.requests().first().options.url.isEmpty());
    }

    void autoNames_data()
    {
        QTest::addColumn<QString>("method");
        QTest::addColumn<QString>("url");
        QTest::addColumn<QString>("name");
        QTest::newRow("path") << "get" << "https://api.example.com/v1/users?x=1" << "GET api.example.com/v1/users";
        QTest::newRow("root") << "POST" << "https://example.com/" << "POST example.com";
        QTest::newRow("no scheme") << "PUT" << "example.com/a" << "PUT example.com/a";
        QTest::newRow("empty") << "GET" << "  " << "";
    }
    void autoNames()
    {
        QFETCH(QString, method);
        QFETCH(QString, url);
        QFETCH(QString, name);
        CurlOptions o;
        o.method = method;
        o.url = url;
        QCOMPARE(RequestStore::autoName(o), name);
    }

    void corruptFileIsKeptAndReplaced()
    {
        writeFile("{ this is not json");
        RequestStore store(path());
        QCOMPARE(store.load(), RequestStore::LoadResult::Corrupt);
        QVERIFY(QFile::exists(store.backupPath()));
        QFile backup(store.backupPath());
        QVERIFY(backup.open(QIODevice::ReadOnly));
        QCOMPARE(backup.readAll(), QByteArray("{ this is not json"));
        QCOMPARE(store.count(), 1);
        QVERIFY(!store.isReadOnly());
        QVERIFY(QFile::exists(path())); // a fresh, valid file
        RequestStore check(path());
        QCOMPARE(check.load(), RequestStore::LoadResult::Loaded);
    }

    void newerVersionIsReadOnly()
    {
        const QByteArray future = "{\"version\": 99, \"requests\": [{\"id\": \"x\", \"name\": \"Future\", \"url\": \"https://f.test\", \"newField\": 1}]}";
        writeFile(future);
        RequestStore store(path());
        QCOMPARE(store.load(), RequestStore::LoadResult::NewerVersion);
        QVERIFY(store.isReadOnly());
        QCOMPARE(store.requests().first().name, QString("Future"));
        store.rename("x", "Changed");
        QVERIFY(!store.save());
        store.flush();
        QFile f(path());
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), future); // untouched
    }

    void toleratesOddContent()
    {
        writeFile("{\"requests\": [\n"
            "            {\"id\": \"dup\", \"url\": 5, \"method\": \"\", \"headers\": [{\"name\": \"\"}, {\"name\": \"A\", \"value\": \"1\"}], \"maxTimeSeconds\": -3},\n"
            "            {\"id\": \"dup\", \"name\": \"second\"},\n"
            "            \"not an object\"\n"
            "        ], \"current\": \"missing\"}");
        RequestStore store(path());
        QCOMPARE(store.load(), RequestStore::LoadResult::Loaded);
        QCOMPARE(store.count(), 3);
        const auto &r = store.requests();
        QCOMPARE(r[0].options.method, QString("GET"));
        QVERIFY(r[0].options.url.isEmpty());
        QCOMPARE(r[0].options.headers.size(), 1);
        QCOMPARE(r[0].options.maxTimeSeconds, 0);
        QVERIFY(r[1].id != r[0].id); // duplicate id replaced
        QCOMPARE(store.currentId(), r[0].id); // missing current: first
    }

    void unreadableFileIsNotOverwritten()
    {
#ifdef Q_OS_WIN
        QSKIP("Permissions can't make a file unreadable here");
#else
        writeFile("{\"version\": 1, \"requests\": []}");
        QFile::setPermissions(path(), QFileDevice::Permissions());
        if (QFile(path()).open(QIODevice::ReadOnly))
            QSKIP("Running as a user who can read any file (e.g. root)");
        RequestStore store(path());
        QCOMPARE(store.load(), RequestStore::LoadResult::Inaccessible);
        QVERIFY(store.isReadOnly());
        QVERIFY(!store.save());
        QFile::setPermissions(path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif
    }

    void saveFailureIsReported()
    {
        // A file where the parent directory should be makes writing impossible.
        QFile blocker(m_dir.filePath("sub"));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        RequestStore store(path());
        QCOMPARE(store.load(), RequestStore::LoadResult::New);
        QSignalSpy failed(&store, &RequestStore::saveFailed);
        QVERIFY(!store.save());
        QCOMPARE(failed.count(), 1);
    }

    void defaultPathUsesAppData()
    {
        QVERIFY(RequestStore::defaultPath().endsWith("/library.json"));
    }
};

QTEST_GUILESS_MAIN(TestRequestStore)
#include "tst_requeststore.moc"
