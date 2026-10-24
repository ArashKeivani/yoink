/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ShelfPlacement.h"

#include <algorithm>

namespace
{
constexpr int StageWidth = 356;
constexpr int StageHeight = 456;
constexpr int CardMargin = 10;
constexpr int HandleLength = 96;
constexpr int HandleThickness = 8;
constexpr int HandleInputLength = 120;
constexpr int HandleInputThickness = 24;

QRect centeredVerticalHandle(const QSize &stageSize, bool rightSide)
{
    const int length = std::min(HandleLength, stageSize.height());
    const int x = rightSide ? stageSize.width() - HandleThickness : 0;
    return QRect(x, (stageSize.height() - length) / 2, HandleThickness, length);
}

QRect centeredHorizontalHandle(const QSize &stageSize, bool bottomSide)
{
    const int length = std::min(HandleLength, stageSize.width());
    const int y = bottomSide ? stageSize.height() - HandleThickness : 0;
    return QRect((stageSize.width() - length) / 2, y, length, HandleThickness);
}

QRect verticalHitArea(const QSize &stageSize, bool rightSide)
{
    const int width = std::min(HandleInputThickness, stageSize.width());
    const int height = std::min(HandleInputLength, stageSize.height());
    const int x = rightSide ? stageSize.width() - width : 0;
    return QRect(x, (stageSize.height() - height) / 2, width, height);
}

QRect horizontalHitArea(const QSize &stageSize, bool bottomSide)
{
    const int width = std::min(HandleInputLength, stageSize.width());
    const int height = std::min(HandleInputThickness, stageSize.height());
    const int y = bottomSide ? stageSize.height() - height : 0;
    return QRect((stageSize.width() - width) / 2, y, width, height);
}
} // namespace

ShelfGeometry::Layout ShelfGeometry::computeLayout(const QRect &screenGeometry, const QRect &availableGeometry, Edge edge, int positionPercent)
{
    const QRect available = availableGeometry.isValid() ? availableGeometry : screenGeometry;
    const int percent = std::clamp(positionPercent, 0, 100);

    Layout layout;
    layout.edge = edge;
    layout.verticalEdge = edge == Edge::Left || edge == Edge::Right;
    layout.stageSize = QSize(std::min(StageWidth, std::max(1, available.width())), std::min(StageHeight, std::max(1, available.height())));
    layout.cardSize = QSize(std::max(1, layout.stageSize.width() - 2 * CardMargin), std::max(1, layout.stageSize.height() - 2 * CardMargin));
    layout.cardRect = QRect(CardMargin, CardMargin, layout.cardSize.width(), layout.cardSize.height());

    int x = available.left();
    int y = available.top();
    if (layout.verticalEdge) {
        y += qRound((available.height() - layout.stageSize.height()) * (percent / 100.0));
    } else {
        x += qRound((available.width() - layout.stageSize.width()) * (percent / 100.0));
    }

    const int panelLeft = available.left() - screenGeometry.left();
    const int panelTop = available.top() - screenGeometry.top();
    const int panelRight = screenGeometry.right() - available.right();
    const int panelBottom = screenGeometry.bottom() - available.bottom();

    switch (edge) {
    case Edge::Left:
        layout.margins = QMargins(panelLeft, y - screenGeometry.top(), 0, 0);
        layout.handleRect = centeredVerticalHandle(layout.stageSize, false);
        layout.handleHitRect = verticalHitArea(layout.stageSize, false);
        break;
    case Edge::Right:
        layout.margins = QMargins(0, y - screenGeometry.top(), panelRight, 0);
        layout.handleRect = centeredVerticalHandle(layout.stageSize, true);
        layout.handleHitRect = verticalHitArea(layout.stageSize, true);
        break;
    case Edge::Top:
        layout.margins = QMargins(x - screenGeometry.left(), panelTop, 0, 0);
        layout.handleRect = centeredHorizontalHandle(layout.stageSize, false);
        layout.handleHitRect = horizontalHitArea(layout.stageSize, false);
        break;
    case Edge::Bottom:
        layout.margins = QMargins(x - screenGeometry.left(), 0, 0, panelBottom);
        layout.handleRect = centeredHorizontalHandle(layout.stageSize, true);
        layout.handleHitRect = horizontalHitArea(layout.stageSize, true);
        break;
    }
    return layout;
}

QRegion ShelfGeometry::collapsedInputRegion(const Layout &layout)
{
    return QRegion(layout.handleHitRect);
}

QRegion ShelfGeometry::expandedInputRegion(const Layout &layout)
{
    return QRegion(QRect(QPoint(0, 0), layout.stageSize));
}
