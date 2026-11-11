// Takes the product screenshots used on the website, in light and dark
// themes, English and Japanese, with and without the library:
//
//   QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 ./kurly-screenshots <output-dir>
//
// It drives Kurly's real main window, with Qt's Fusion style and its standard
// light or dark colors (what Kurly looks like on macOS in each mode), and a
// stand-in curl that returns a sample JSON response, so the screenshots show
// "api.example.com" instead of a real server. Writes PNGs named
// <screen>-<lang>-<theme>.png (screen: "main" or "library").
//
// Build with -DKURLY_BUILD_SCREENSHOT_TOOL=ON. Unix only (the stand-in curl is
// a shell script).

#include "mainwindow.h"
#include "requeststore.h"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QStyle>
#include <QStyleFactory>
#include <QTemporaryDir>
#include <QThread>

#include <cstdio>

namespace {

// Qt's Fusion palette for dark appearance (as Qt 6.5+ uses on macOS).
QPalette fusionDarkPalette()
{
    const QColor windowText(240, 240, 240);
    const QColor background(50, 50, 50);
    const QColor light = background.lighter(150);
    const QColor mid = background.darker(130);
    const QColor dark = background.darker(150);
    const QColor base = background.darker(140);
    const QColor disabledText(130, 130, 130);

    QPalette p(windowText, background, light, dark, mid, windowText, base);
    p.setBrush(QPalette::Midlight, mid.lighter(110));
    p.setBrush(QPalette::Button, background);
    p.setBrush(QPalette::Shadow, dark.darker(135));
    p.setBrush(QPalette::Highlight, QColor(48, 140, 198));
    p.setBrush(QPalette::HighlightedText, windowText);
    p.setBrush(QPalette::Link, QColor(48, 140, 198));
    p.setBrush(QPalette::AlternateBase, background);
    QColor placeholder = windowText;
    placeholder.setAlpha(128);
    p.setBrush(QPalette::PlaceholderText, placeholder);
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        p.setBrush(QPalette::Disabled, role, disabledText);
    p.setBrush(QPalette::Disabled, QPalette::Base, background);
    return p;
}

QJsonObject request(const QString &id, const QString &name, const CurlOptions &o)
{
    QJsonObject json = RequestStore::toJson(o);
    json.insert("id", id);
    json.insert("name", name);
    return json;
}

CurlOptions userRequest()
{
    CurlOptions o;
    o.url = "https://api.example.com/v1/users/42?include=projects";
    o.headers = {{"accept", "application/json"},
                 {"accept-language", "ja,en;q=0.9"},
                 {"authorization", "Bearer YOUR_TOKEN"}};
    o.compressed = true;
    return o;
}

// A library file: just the user request, or five requests.
QString writeLibrary(const QString &dir, bool full)
{
    QJsonArray requests{request("r1", "", userRequest())};
    if (full) {
        CurlOptions create;
        create.method = "POST";
        create.url = "https://api.example.com/v1/users";
        create.headers = {{"content-type", "application/json"}};
        create.body = "{\"name\": \"Hanako Yamada\"}";
        CurlOptions logout;
        logout.method = "DELETE";
        logout.url = "https://api.example.com/v1/sessions/current";
        logout.headers = {{"authorization", "Bearer YOUR_TOKEN"}};
        CurlOptions health;
        health.url = "https://status.example.com/health";
        CurlOptions star;
        star.method = "PATCH";
        star.url = "https://api.example.com/v1/projects/7";
        star.headers = {{"content-type", "application/json"}};
        star.body = "{\"stars\": 129}";
        requests.append(request("r2", "Create user", create));
        requests.append(request("r3", "", logout));
        requests.append(request("r4", "Health check", health));
        requests.append(request("r5", "", star));
    }
    const QString path = dir + (full ? "/library-full.json" : "/library-one.json");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        qFatal("can't write %s", qPrintable(path));
    f.write(QJsonDocument(QJsonObject{{"version", 1}, {"current", "r1"}, {"requests", requests}})
                .toJson());
    return path;
}

QString writeFakeCurl(const QString &dir)
{
    const QString path = dir + "/curl";
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        qFatal("can't write %s", qPrintable(path));
    f.write("#!/bin/sh\n"
            "# Stand-in for curl: a sample API response.\n"
            "sleep 0.15\n"
            "printf '%s' '{\"id\":42,\"name\":\"Hanako Yamada\",\"email\":\"hanako@example.com\","
            "\"roles\":[\"admin\",\"editor\"],\"settings\":{\"theme\":\"dark\",\"language\":\"ja\","
            "\"notifications\":true},\"projects\":[{\"id\":7,\"name\":\"kurly\",\"stars\":128},"
            "{\"id\":9,\"name\":\"docs\",\"stars\":12}],\"created_at\":\"2026-04-01T09:30:00Z\"}'\n");
    f.close();
    f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    return path;
}

void waitFor(const std::function<bool()> &done, int timeoutMs = 10000)
{
    QElapsedTimer timer;
    timer.start();
    while (!done() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(10);
    }
    if (!done())
        qFatal("timed out");
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <output-dir>\n", argv[0]);
        return 2;
    }
    const QString out = QString::fromLocal8Bit(argv[1]);
    QDir().mkpath(out);

    // Keep away from the real settings and library.
    QCoreApplication::setOrganizationName("Kurly Screenshots");
    QCoreApplication::setApplicationName("kurly-screenshots");
    QStandardPaths::setTestModeEnabled(true);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    const QPalette lightPalette = QApplication::style()->standardPalette();
    const QPalette darkPalette = fusionDarkPalette();

    QTemporaryDir tmp;
    const QString fakeCurl = writeFakeCurl(tmp.path());

    struct Screen { const char *name; bool library; QSize size; };
    const Screen screens[] = {{"main", false, {1120, 720}}, {"library", true, {1400, 750}}};

    for (const char *lang : {"en", "ja"}) {
        for (const char *theme : {"light", "dark"}) {
            QApplication::setPalette(QString(theme) == "dark" ? darkPalette : lightPalette);
            for (const Screen &screen : screens) {
                QSettings settings;
                settings.clear();
                settings.setValue("language", lang);
                settings.setValue("prettyPrint", true);
                settings.setValue("libraryVisible", screen.library);
                settings.sync();

                MainWindow w(nullptr, writeLibrary(tmp.path(), screen.library));
                w.setCurlProgram(fakeCurl);
                w.resize(screen.size);
                w.show();
                QCoreApplication::processEvents();

                auto *run = w.findChild<QPushButton *>("run");
                auto *status = w.findChild<QLabel *>("status");
                run->click();
                waitFor([status] { return !status->text().isEmpty() && !status->text().contains("…"); });
                QCoreApplication::processEvents();

                const QString file = QStringLiteral("%1/%2-%3-%4.png").arg(out, screen.name, lang, theme);
                if (!w.grab().save(file))
                    qFatal("can't save %s", qPrintable(file));
                std::printf("%s\n", qPrintable(file));
            }
        }
    }
    return 0;
}
