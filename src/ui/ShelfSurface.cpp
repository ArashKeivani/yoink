/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ShelfSurface.h"

#include <LayerShellQt/Window>

#include <QEvent>
#include <QIcon>

ShelfSurface::ShelfSurface(QWindow *parent)
    : QQuickView(parent)
{
    setFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setColor(Qt::transparent);
    setResizeMode(QQuickView::SizeRootObjectToView);
    setTitle(QStringLiteral("Yoink"));
    setIcon(QIcon(QStringLiteral(":/icons/org.yoink.Yoink.svg")));
}

bool ShelfSurface::initializeLayerShell()
{
    m_layer = LayerShellQt::Window::get(this);
    if (!m_layer) {
        return false;
    }

    m_layer->setScope(QStringLiteral("yoink"));
    m_layer->setLayer(LayerShellQt::Window::LayerTop);
    m_layer->setExclusiveZone(0);
    m_layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
    m_layer->setActivateOnShow(false);
    m_layer->setCloseOnDismissed(false);
    m_layer->setAnchors(LayerShellQt::Window::Anchors{LayerShellQt::Window::AnchorLeft, LayerShellQt::Window::AnchorTop});
    return true;
}

void ShelfSurface::applyLayout(const ShelfGeometry::Layout &layout, QScreen *screen)
{
    m_layout = layout;
    m_targetScreen = screen;

    if (m_layer) {
        LayerShellQt::Window::Anchors anchors;
        switch (layout.edge) {
        case ShelfGeometry::Edge::Left:
            anchors = {LayerShellQt::Window::AnchorLeft, LayerShellQt::Window::AnchorTop};
            break;
        case ShelfGeometry::Edge::Right:
            anchors = {LayerShellQt::Window::AnchorRight, LayerShellQt::Window::AnchorTop};
            break;
        case ShelfGeometry::Edge::Top:
            anchors = {LayerShellQt::Window::AnchorTop, LayerShellQt::Window::AnchorLeft};
            break;
        case ShelfGeometry::Edge::Bottom:
            anchors = {LayerShellQt::Window::AnchorBottom, LayerShellQt::Window::AnchorLeft};
            break;
        }
        if (screen) {
            m_layer->setScreen(screen);
        }
        m_layer->setAnchors(anchors);
        m_layer->setMargins(layout.margins);
    }

    if (size() != layout.stageSize) {
        resize(layout.stageSize);
    }
    updateInputRegion();
}

void ShelfSurface::setInputExpanded(bool expanded)
{
    if (m_inputExpanded == expanded) {
        return;
    }
    m_inputExpanded = expanded;
    updateInputRegion();
}

bool ShelfSurface::inputExpanded() const
{
    return m_inputExpanded;
}

const ShelfGeometry::Layout &ShelfSurface::layout() const
{
    return m_layout;
}

QScreen *ShelfSurface::targetScreen() const
{
    return m_targetScreen;
}

bool ShelfSurface::event(QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        if (!m_pointerInside) {
            m_pointerInside = true;
            Q_EMIT pointerInsideChanged(true);
        }
    } else if (event->type() == QEvent::Leave) {
        if (m_pointerInside) {
            m_pointerInside = false;
            Q_EMIT pointerInsideChanged(false);
        }
    }
    return QQuickView::event(event);
}

void ShelfSurface::updateInputRegion()
{
    if (!handle()) {
        return;
    }
    setMask(m_inputExpanded ? ShelfGeometry::expandedInputRegion(m_layout) : ShelfGeometry::collapsedInputRegion(m_layout));
}
