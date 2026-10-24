/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KStatusNotifierItem>

class QAction;
class QMenu;

class TrayIcon : public KStatusNotifierItem
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() override;

    void setExpanded(bool expanded);
    void setAutostartEnabled(bool enabled);

Q_SIGNALS:
    void toggleRequested();
    void settingsRequested();
    void quitRequested();
    void autostartToggleRequested(bool enabled);

private:
    QMenu *m_menu = nullptr;
    QAction *m_showAction = nullptr;
    QAction *m_autostartAction = nullptr;
};
