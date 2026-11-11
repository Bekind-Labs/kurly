#include "requeststore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QUuid>

namespace {

QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// Flags stored under "options", in a fixed order.
struct Flag
{
    const char *key;
    bool CurlOptions::*member;
};
constexpr Flag kFlags[] = {
    {"followRedirects", &CurlOptions::followRedirects},
    {"insecure", &CurlOptions::insecure},
    {"verbose", &CurlOptions::verbose},
    {"silent", &CurlOptions::silent},
    {"includeHeaders", &CurlOptions::includeHeaders},
    {"compressed", &CurlOptions::compressed},
    {"failOnError", &CurlOptions::failOnError},
};

} // namespace

RequestStore::RequestStore(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_path(filePath)
    , m_saveTimer(new QTimer(this))
{
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(400);
    connect(m_saveTimer, &QTimer::timeout, this, &RequestStore::save);
}

RequestStore::~RequestStore()
{
    flush();
}

QString RequestStore::defaultPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/library.json");
}

void RequestStore::setSaveDelay(int ms)
{
    m_saveTimer->setInterval(ms);
}

// ---- JSON --------------------------------------------------------------------

QJsonObject RequestStore::toJson(const CurlOptions &o)
{
    QJsonArray headers;
    for (const auto &h : o.headers)
        headers.append(QJsonObject{{"name", h.first}, {"value", h.second}});
    QJsonObject flags;
    for (const auto &f : kFlags)
        flags.insert(QLatin1String(f.key), o.*(f.member));
    return QJsonObject{
        {"method", o.method},
        {"url", o.url},
        {"headers", headers},
        {"body", o.body},
        {"user", o.user},
        {"userAgent", o.userAgent},
        {"outputFile", o.outputFile},
        {"maxTimeSeconds", o.maxTimeSeconds},
        {"options", flags},
    };
}

CurlOptions RequestStore::optionsFromJson(const QJsonObject &json)
{
    // Wrong types fall back to defaults instead of failing the whole file.
    CurlOptions o;
    o.method = json.value("method").toString(o.method);
    if (o.method.trimmed().isEmpty())
        o.method = QStringLiteral("GET");
    o.url = json.value("url").toString();
    for (const auto &value : json.value("headers").toArray()) {
        const QJsonObject h = value.toObject();
        const QString name = h.value("name").toString();
        if (!name.trimmed().isEmpty())
            o.headers.append({name, h.value("value").toString()});
    }
    o.body = json.value("body").toString();
    o.user = json.value("user").toString();
    o.userAgent = json.value("userAgent").toString();
    o.outputFile = json.value("outputFile").toString();
    o.maxTimeSeconds = qMax(0, json.value("maxTimeSeconds").toInt());
    const QJsonObject flags = json.value("options").toObject();
    for (const auto &f : kFlags)
        o.*(f.member) = flags.value(QLatin1String(f.key)).toBool();
    return o;
}

// ---- Loading and saving --------------------------------------------------------

RequestStore::LoadResult RequestStore::load()
{
    m_requests.clear();
    m_currentId.clear();
    m_readOnly = false;
    m_backupPath.clear();

    QFile file(m_path);
    if (!file.exists()) {
        ensureNotEmpty();
        return LoadResult::New;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        // Don't overwrite what we can't read.
        m_readOnly = true;
        ensureNotEmpty();
        return LoadResult::Inaccessible;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        // Keep the damaged file for the user, then start over.
        m_backupPath = m_path + QStringLiteral(".corrupt-")
                       + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
        QFile::rename(m_path, m_backupPath);
        ensureNotEmpty();
        save();
        return LoadResult::Corrupt;
    }

    const QJsonObject root = doc.object();
    const int version = root.value("version").toInt(1);

    QSet<QString> seen;
    for (const auto &value : root.value("requests").toArray()) {
        const QJsonObject json = value.toObject();
        SavedRequest r;
        r.id = json.value("id").toString();
        if (r.id.isEmpty() || seen.contains(r.id))
            r.id = newId();
        seen.insert(r.id);
        r.name = json.value("name").toString();
        r.options = optionsFromJson(json);
        m_requests.append(r);
    }
    m_currentId = root.value("current").toString();
    ensureNotEmpty();

    if (version > kFormatVersion) {
        m_readOnly = true; // saving would drop whatever the newer version added
        return LoadResult::NewerVersion;
    }
    return LoadResult::Loaded;
}

bool RequestStore::save()
{
    m_saveTimer->stop();
    if (m_readOnly)
        return false;

    QJsonArray requests;
    for (const auto &r : m_requests) {
        QJsonObject json = toJson(r.options);
        json.insert("id", r.id);
        json.insert("name", r.name);
        requests.append(json);
    }
    const QJsonObject root{
        {"version", kFormatVersion},
        {"current", m_currentId},
        {"requests", requests},
    };

    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path); // writes a temporary file, then renames it into place
    if (!file.open(QIODevice::WriteOnly)) {
        emit saveFailed(file.errorString());
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        emit saveFailed(file.errorString());
        return false;
    }
    // Requests can contain passwords and tokens.
    QFile::setPermissions(m_path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

void RequestStore::flush()
{
    if (m_saveTimer->isActive())
        save();
}

// ---- Editing -------------------------------------------------------------------

int RequestStore::indexOf(const QString &id) const
{
    for (int i = 0; i < m_requests.size(); ++i)
        if (m_requests.at(i).id == id)
            return i;
    return -1;
}

const SavedRequest *RequestStore::find(const QString &id) const
{
    const int i = indexOf(id);
    return i < 0 ? nullptr : &m_requests.at(i);
}

void RequestStore::setCurrentId(const QString &id)
{
    if (id == m_currentId || indexOf(id) < 0)
        return;
    m_currentId = id;
    m_saveTimer->start(); // remembered for next launch
    emit currentChanged(id);
}

QString RequestStore::add(const CurlOptions &options, const QString &name, const QString &afterId)
{
    SavedRequest r{newId(), name.trimmed(), stored(options)};
    const int after = indexOf(afterId);
    m_requests.insert(after < 0 ? m_requests.size() : after + 1, r);
    changed();
    emit listChanged();
    return r.id;
}

QString RequestStore::duplicate(const QString &id)
{
    const SavedRequest *r = find(id);
    if (!r)
        return {};
    return add(r->options, r->name, id);
}

void RequestStore::remove(const QString &id)
{
    const int i = indexOf(id);
    if (i < 0)
        return;
    m_requests.removeAt(i);
    const bool wasCurrent = id == m_currentId;
    ensureNotEmpty();
    if (wasCurrent)
        m_currentId = m_requests.at(qMin(i, int(m_requests.size()) - 1)).id;
    changed();
    emit listChanged();
    if (wasCurrent)
        emit currentChanged(m_currentId);
}

void RequestStore::rename(const QString &id, const QString &name)
{
    const int i = indexOf(id);
    if (i < 0 || m_requests[i].name == name.trimmed())
        return;
    m_requests[i].name = name.trimmed();
    changed();
    emit requestChanged(id);
}

void RequestStore::setOptions(const QString &id, const CurlOptions &options)
{
    const int i = indexOf(id);
    const CurlOptions value = stored(options);
    if (i < 0 || m_requests[i].options == value)
        return;
    m_requests[i].options = value;
    changed();
    emit requestChanged(id);
}

void RequestStore::move(int from, int to)
{
    if (from < 0 || from >= m_requests.size() || to < 0 || to >= m_requests.size() || from == to)
        return;
    m_requests.move(from, to);
    changed();
    emit listChanged();
}

QString RequestStore::autoName(const CurlOptions &options)
{
    const QString url = options.url.trimmed();
    if (url.isEmpty())
        return {};
    const QUrl parsed = QUrl::fromUserInput(url);
    QString where = parsed.isValid() && !parsed.host().isEmpty()
                        ? parsed.host() + parsed.path()
                        : url;
    if (where.endsWith(QLatin1Char('/')) && where.count(QLatin1Char('/')) == 1)
        where.chop(1); // "example.com/" → "example.com"
    return options.method.trimmed().toUpper() + QLatin1Char(' ') + where;
}

// ---- Helpers -------------------------------------------------------------------

void RequestStore::changed()
{
    m_saveTimer->start();
}

void RequestStore::ensureNotEmpty()
{
    if (m_requests.isEmpty())
        m_requests.append({newId(), QString(), CurlOptions()});
    if (indexOf(m_currentId) < 0)
        m_currentId = m_requests.first().id;
}

CurlOptions RequestStore::stored(CurlOptions options)
{
    // Display preferences belong to the window, not the request.
    options.longOptions = false;
    options.multiline = false;
    return options;
}
