/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QLocalServer>
#include <QLockFile>
#include <QObject>

class QLocalSocket;

class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QString id, QObject *parent = nullptr);
    ~SingleInstance() override;

    [[nodiscard]] bool tryAcquire();
    [[nodiscard]] bool sendMessage(const QString &message);

Q_SIGNALS:
    void messageReceived(const QString &message);

private:
    void handleNewConnection();
    void processSocket(QLocalSocket *socket);

    QString m_id;
    QLockFile m_lock;
    QLocalServer m_server;
    bool m_acquired = false;
};
