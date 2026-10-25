/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QQuickItem>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;

class DropSurface : public QQuickItem
{
    Q_OBJECT

public:
    explicit DropSurface(QQuickItem *parent = nullptr);
    void setInternalDragActive(bool active);

Q_SIGNALS:
    void externalDragEntered(const QString &summary, int count);
    void externalDragLeft();
    void mimeDropped(const QMimeData *mimeData);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool m_dragAccepted = false;
    bool m_internalDragActive = false;
};
