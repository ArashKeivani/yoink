/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KConfigGroup>
#include <KSharedConfig>

#include <QObject>
#include <QTimer>

class AppSettings : public QObject
{
    Q_OBJECT

public:
    enum Edge {
        Left = 0,
        Right = 1,
        Top = 2,
        Bottom = 3,
    };
    Q_ENUM(Edge)

    Q_PROPERTY(int edge READ edge WRITE setEdge NOTIFY edgeChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(int toastDuration READ toastDuration WRITE setToastDuration NOTIFY toastDurationChanged)
    Q_PROPERTY(QString screenName READ screenName WRITE setScreenName NOTIFY screenNameChanged)
    Q_PROPERTY(bool pinned READ pinned WRITE setPinned NOTIFY pinnedChanged)

    explicit AppSettings(QObject *parent = nullptr);
    explicit AppSettings(KSharedConfig::Ptr config, QObject *parent = nullptr);
    ~AppSettings() override;

    [[nodiscard]] int edge() const;
    void setEdge(int edge);

    [[nodiscard]] int position() const;
    void setPosition(int position);

    [[nodiscard]] int toastDuration() const;
    void setToastDuration(int durationMs);

    [[nodiscard]] QString screenName() const;
    void setScreenName(const QString &screenName);

    [[nodiscard]] bool pinned() const;
    void setPinned(bool pinned);

    Q_INVOKABLE void flush();

Q_SIGNALS:
    void edgeChanged();
    void positionChanged();
    void toastDurationChanged();
    void screenNameChanged();
    void pinnedChanged();
    void saveFailed(const QString &message);

private:
    void initialize();
    void scheduleSync();

    KSharedConfig::Ptr m_config;
    KConfigGroup m_group;
    QTimer m_syncTimer;
    int m_edge = Right;
    int m_position = 50;
    int m_toastDuration = 2500;
    QString m_screenName;
    bool m_pinned = false;
};
