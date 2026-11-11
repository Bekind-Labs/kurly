#pragma once

#include "curlcommand.h"

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>

class QTimer;

// A request saved in the library.
struct SavedRequest
{
    QString id;   // stable, unique (a UUID)
    QString name; // empty: shown with an automatic name (see autoName())
    CurlOptions options; // longOptions/multiline are display preferences and not stored
};

// The user's library of requests, persisted as a JSON file.
//
// Changes are saved automatically shortly after they happen (and on
// flush()/destruction). Files are written atomically and readable only by
// the owner, since requests can contain credentials. There is always at
// least one request, and always a current one. GUI-independent.
class RequestStore : public QObject
{
    Q_OBJECT
public:
    enum class LoadResult {
        Loaded,        // read the file
        New,           // no file yet: started with one empty request
        Corrupt,       // unreadable JSON: moved aside (backupPath()), started fresh
        NewerVersion,  // written by a newer Kurly: loaded what we could, read-only
        Inaccessible,  // exists but can't be opened: started empty, read-only
    };

    static constexpr int kFormatVersion = 1;

    explicit RequestStore(const QString &filePath, QObject *parent = nullptr);
    ~RequestStore() override; // saves pending changes

    // <app data dir>/library.json
    static QString defaultPath();

    LoadResult load();
    QString filePath() const { return m_path; }
    QString backupPath() const { return m_backupPath; }
    bool isReadOnly() const { return m_readOnly; }

    // Saving. Mutations schedule a save automatically.
    bool save();
    void flush(); // save now if a save is pending
    void setSaveDelay(int ms);

    const QList<SavedRequest> &requests() const { return m_requests; }
    int count() const { return int(m_requests.size()); }
    int indexOf(const QString &id) const;
    const SavedRequest *find(const QString &id) const;

    QString currentId() const { return m_currentId; }
    void setCurrentId(const QString &id);

    // Returns the new request's id. Inserted after `afterId` (or at the end).
    QString add(const CurlOptions &options = {}, const QString &name = {},
                const QString &afterId = {});
    QString duplicate(const QString &id);
    // Removing the last request leaves a new empty one in its place.
    void remove(const QString &id);
    void rename(const QString &id, const QString &name);
    void setOptions(const QString &id, const CurlOptions &options);
    void move(int from, int to);

    // "GET api.example.com/v1/users" from the method and URL; empty if there
    // is no URL.
    static QString autoName(const CurlOptions &options);

    static QJsonObject toJson(const CurlOptions &options);
    static CurlOptions optionsFromJson(const QJsonObject &json);

signals:
    void listChanged();                     // added, removed or reordered
    void requestChanged(const QString &id); // a request's options or name changed
    void currentChanged(const QString &id);
    void saveFailed(const QString &error);

private:
    void changed();
    void ensureNotEmpty();
    static CurlOptions stored(CurlOptions options);

    QString m_path;
    QString m_backupPath;
    QList<SavedRequest> m_requests;
    QString m_currentId;
    bool m_readOnly = false;
    QTimer *m_saveTimer;
};
