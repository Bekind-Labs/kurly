#pragma once

// Shared by the widget tests.

#include <QHash>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

// A tiny HTTP server for exercising Run without the internet.
class TestServer : public QTcpServer
{
public:
    enum class Mode { Echo, Hang, NotFound, Big, Json };
    static constexpr const char *kJson = "{\"zebra\":1,\"apple\":[true,null],\"name\":\"テスト\"}";
    static constexpr const char *kJsonPretty =
        "{\n  \"zebra\": 1,\n  \"apple\": [\n    true,\n    null\n  ],\n  \"name\": \"テスト\"\n}\n";
    Mode mode = Mode::Echo;
    static constexpr int kBigSize = 3 * 1024 * 1024;

    TestServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *s = nextPendingConnection()) {
                connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
                connect(s, &QTcpSocket::readyRead, this, [this, s] { handle(s); });
            }
        });
    }

    QString url(const QString &path = QStringLiteral("/echo")) const
    {
        return QStringLiteral("http://127.0.0.1:%1%2").arg(serverPort()).arg(path);
    }

private:
    QHash<QTcpSocket *, QByteArray> m_buffers;

    void handle(QTcpSocket *s)
    {
        QByteArray &buf = m_buffers[s];
        buf += s->readAll();
        const int end = buf.indexOf("\r\n\r\n");
        if (end < 0)
            return;
        const QByteArray head = buf.left(end);
        int length = 0;
        for (const QByteArray &line : head.split('\n'))
            if (line.toLower().startsWith("content-length:"))
                length = line.mid(15).trimmed().toInt();
        if (buf.size() < end + 4 + length)
            return;
        const QByteArray body = buf.mid(end + 4, length);
        m_buffers.remove(s);

        QByteArray status = "200 OK", payload;
        switch (mode) {
        case Mode::Hang:
            return; // never answer
        case Mode::NotFound:
            status = "404 Not Found";
            payload = "nope";
            break;
        case Mode::Big:
            payload = QByteArray(kBigSize, 'x'); // one huge line, like minified JSON
            break;
        case Mode::Json:
            payload = kJson;
            break;
        case Mode::Echo:
            payload = "request:\n" + head + "\nbody=" + body + "\n";
            break;
        }
        s->write("HTTP/1.1 " + status + "\r\nContent-Type: text/plain; charset=utf-8\r\n"
                 "Content-Length: " + QByteArray::number(payload.size())
                 + "\r\nConnection: close\r\n\r\n" + payload);
        s->disconnectFromHost();
    }
};
