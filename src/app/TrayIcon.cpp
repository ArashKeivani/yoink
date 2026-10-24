/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "TrayIcon.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QSignalBlocker>

TrayIcon::TrayIcon(QObject *parent)
    : KStatusNotifierItem(parent)
{
    setCategory(KStatusNotifierItem::ApplicationStatus);
    setStatus(KStatusNotifierItem::Active);
    setTitle(QStringLiteral("Yoink"));
    setIconByPixmap(QIcon(QStringLiteral(":/icons/org.yoink.Yoink.svg")));
    setToolTip(QIcon(QStringLiteral(":/icons/org.yoink.Yoink.svg")), QStringLiteral("Yoink"), tr("Drag and drop shelf"));
    setStandardActionsEnabled(false);

    m_menu = new QMenu;
    m_showAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("view-visible")), tr("Show Yoink"));
    connect(m_showAction, &QAction::triggered, this, &TrayIcon::toggleRequested);

    QAction *settingsAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("configure")), tr("Settings…"));
    connect(settingsAction, &QAction::triggered, this, &TrayIcon::settingsRequested);

    m_menu->addSeparator();
    m_autostartAction = m_menu->addAction(tr("Start at Login"));
    m_autostartAction->setCheckable(true);
    connect(m_autostartAction, &QAction::toggled, this, &TrayIcon::autostartToggleRequested);

    m_menu->addSeparator();
    QAction *quitAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), tr("Quit Yoink"));
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);

    setContextMenu(m_menu);
    connect(this, &KStatusNotifierItem::activateRequested, this, [this](bool, const QPoint &) { Q_EMIT toggleRequested(); });
    connect(this, &KStatusNotifierItem::secondaryActivateRequested, this, &TrayIcon::settingsRequested);
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
}

void TrayIcon::setExpanded(bool expanded)
{
    if (m_showAction) {
        m_showAction->setText(expanded ? tr("Hide Yoink") : tr("Show Yoink"));
    }
}

void TrayIcon::setAutostartEnabled(bool enabled)
{
    if (!m_autostartAction) {
        return;
    }
    const QSignalBlocker blocker(m_autostartAction);
    m_autostartAction->setChecked(enabled);
}
