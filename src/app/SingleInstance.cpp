/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "SingleInstance.h"

#include <QDebug>
#include <QDir>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

#include <utility>

namespace
{
QString lockFilePath(const QString &id)
{
    QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtime.isEmpty()) {
        runtime = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/yoink");
    }
    if (runtime.isEmpty()) {
        runtime = QDir::tempPath();
    }
    QDir().mkpath(runtime);
    return QDir(runtime).filePath(id + QStringLiteral(".lock"));
}
} // namespace

SingleInstance::SingleInstance(QString id, QObject *parent)
    : QObject(parent)
    , m_id(std::move(id))
    , m_lock(lockFilePath(m_id))
{
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
}

SingleInstance::~SingleInstance()
{
    m_server.close();
    if (m_acquired) {
        m_lock.unlock();
    }
}

bool SingleInstance::tryAcquire()
{
    if (!m_lock.tryLock(100)) {
        return false;
    }
    m_acquired = true;

    QLocalServer::removeServer(m_id);
    if (!m_server.listen(m_id)) {
        qWarning() << "Yoink: couldn't create the single-instance socket:" << m_server.errorString();
        m_server.close();
        m_lock.unlock();
        m_acquired = false;
        return false;
    }

    connect(&m_server, &QLocalServer::newConnection, this, &SingleInstance::handleNewConnection);
    return true;
}

bool SingleInstance::sendMessage(const QString &message)
{
    for (int attempt = 0; attempt < 4; ++attempt) {
        QLocalSocket socket;
        socket.connectToServer(m_id);
        if (socket.waitForConnected(500)) {
            const QByteArray payload = message.toUtf8() + '\n';
            if (socket.write(payload) == payload.size()) {
                socket.waitForBytesWritten(1000);
                socket.waitForReadyRead(1000);
                socket.disconnectFromServer();
                return true;
            }
            socket.disconnectFromServer();
            return false;
        }
        if (attempt < 3) {
            QThread::msleep(100);
        }
    }
    return false;
}

void SingleInstance::handleNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QLocalSocket *socket = m_server.nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] { processSocket(socket); });
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        QTimer::singleShot(2000, socket, [socket] {
            if (socket->state() != QLocalSocket::UnconnectedState) {
                socket->disconnectFromServer();
            }
        });
    }
}

void SingleInstance::processSocket(QLocalSocket *socket)
{
    if (socket->property("yoinkHandled").toBool()) {
        return;
    }
    const QByteArray data = socket->readAll();
    if (data.isEmpty()) {
        return;
    }
    socket->setProperty("yoinkHandled", true);

    const QString message = QString::fromUtf8(data).section(QLatin1Char('\n'), 0, 0).trimmed();
    if (!message.isEmpty()) {
        Q_EMIT messageReceived(message);
    }

    socket->write("ok\n");
    socket->flush();
    socket->disconnectFromServer();
}
