/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "AppSettings.h"

#include <QCoreApplication>

#include <utility>

namespace
{
QString edgeToString(int edge)
{
    switch (edge) {
    case AppSettings::Left:
        return QStringLiteral("left");
    case AppSettings::Right:
        return QStringLiteral("right");
    case AppSettings::Top:
        return QStringLiteral("top");
    case AppSettings::Bottom:
        return QStringLiteral("bottom");
    default:
        return QStringLiteral("right");
    }
}

int edgeFromString(const QString &edge)
{
    if (edge == QLatin1String("left")) {
        return AppSettings::Left;
    }
    if (edge == QLatin1String("top")) {
        return AppSettings::Top;
    }
    if (edge == QLatin1String("bottom")) {
        return AppSettings::Bottom;
    }
    return AppSettings::Right;
}
} // namespace

AppSettings::AppSettings(QObject *parent)
    : AppSettings(KSharedConfig::openConfig(), parent)
{
}

AppSettings::AppSettings(KSharedConfig::Ptr config, QObject *parent)
    : QObject(parent)
    , m_config(config ? std::move(config) : KSharedConfig::openConfig())
{
    m_group = KConfigGroup(m_config, QStringLiteral("Shelf"));
    initialize();
}

AppSettings::~AppSettings()
{
    flush();
}

void AppSettings::initialize()
{
    m_edge = edgeFromString(m_group.readEntry(QStringLiteral("edge"), QStringLiteral("right")));
    m_position = qBound(0, m_group.readEntry(QStringLiteral("position"), 50), 100);
    m_toastDuration = qBound(1000, m_group.readEntry(QStringLiteral("toastDuration"), 2500), 15000);
    m_screenName = m_group.readEntry(QStringLiteral("screenName"), QString());
    m_pinned = m_group.readEntry(QStringLiteral("pinned"), false);

    m_syncTimer.setSingleShot(true);
    m_syncTimer.setInterval(250);
    connect(&m_syncTimer, &QTimer::timeout, this, &AppSettings::flush);
}

int AppSettings::edge() const
{
    return m_edge;
}

void AppSettings::setEdge(int edge)
{
    const int bounded = qBound(int(Left), edge, int(Bottom));
    if (m_edge == bounded) {
        return;
    }
    m_edge = bounded;
    m_group.writeEntry(QStringLiteral("edge"), edgeToString(m_edge));
    scheduleSync();
    Q_EMIT edgeChanged();
}

int AppSettings::position() const
{
    return m_position;
}

void AppSettings::setPosition(int position)
{
    const int bounded = qBound(0, position, 100);
    if (m_position == bounded) {
        return;
    }
    m_position = bounded;
    m_group.writeEntry(QStringLiteral("position"), m_position);
    scheduleSync();
    Q_EMIT positionChanged();
}

int AppSettings::toastDuration() const
{
    return m_toastDuration;
}

void AppSettings::setToastDuration(int durationMs)
{
    const int bounded = qBound(1000, durationMs, 15000);
    if (m_toastDuration == bounded) {
        return;
    }
    m_toastDuration = bounded;
    m_group.writeEntry(QStringLiteral("toastDuration"), m_toastDuration);
    scheduleSync();
    Q_EMIT toastDurationChanged();
}

QString AppSettings::screenName() const
{
    return m_screenName;
}

void AppSettings::setScreenName(const QString &screenName)
{
    const QString normalized = screenName.trimmed();
    if (m_screenName == normalized) {
        return;
    }
    m_screenName = normalized;
    m_group.writeEntry(QStringLiteral("screenName"), m_screenName);
    scheduleSync();
    Q_EMIT screenNameChanged();
}

bool AppSettings::pinned() const
{
    return m_pinned;
}

void AppSettings::setPinned(bool pinned)
{
    if (m_pinned == pinned) {
        return;
    }
    m_pinned = pinned;
    m_group.writeEntry(QStringLiteral("pinned"), m_pinned);
    scheduleSync();
    Q_EMIT pinnedChanged();
}

void AppSettings::flush()
{
    if (m_syncTimer.isActive()) {
        m_syncTimer.stop();
    }
    if (!m_config->sync()) {
        Q_EMIT saveFailed(QCoreApplication::translate("AppSettings", "Couldn't save Yoink's settings."));
    }
}

void AppSettings::scheduleSync()
{
    m_syncTimer.start();
}
