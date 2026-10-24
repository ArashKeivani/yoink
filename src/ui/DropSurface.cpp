/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "DropSurface.h"

#include "core/MimeImport.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

DropSurface::DropSurface(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemAcceptsDrops, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

void DropSurface::dragEnterEvent(QDragEnterEvent *event)
{
    const MimeImport::Plan plan = MimeImport::plan(event->mimeData());
    m_dragAccepted = plan.isSupported();
    if (!m_dragAccepted) {
        event->ignore();
        return;
    }

    event->setDropAction(Qt::CopyAction);
    event->accept();
    Q_EMIT externalDragEntered(plan.summary, plan.count);
}

void DropSurface::dragMoveEvent(QDragMoveEvent *event)
{
    if (!m_dragAccepted) {
        event->ignore();
        return;
    }
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void DropSurface::dragLeaveEvent(QDragLeaveEvent *event)
{
    const bool wasAccepted = m_dragAccepted;
    m_dragAccepted = false;
    event->accept();
    if (wasAccepted) {
        Q_EMIT externalDragLeft();
    }
}

void DropSurface::dropEvent(QDropEvent *event)
{
    const MimeImport::Plan plan = MimeImport::plan(event->mimeData());
    m_dragAccepted = false;
    if (!plan.isSupported()) {
        event->ignore();
        return;
    }

    event->setDropAction(Qt::CopyAction);
    event->accept();
    Q_EMIT mimeDropped(event->mimeData());
}
