/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ShelfPlacement.h"
#include <QMargins>
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

static int clampInt(int value, int minimum, int maximum)
{
    return std::clamp(value, minimum, std::max(minimum, maximum));
}
} // namespace

ShelfGeometry::Layout ShelfGeometry::computeLayout(const QRect &screenGeometry, const QRect &availableGeometry, Edge edge, int positionPercent)
{
    const QRect available = availableGeometry.isValid() ? availableGeometry : screenGeometry;
    const int percent = clampInt(positionPercent, 0, 100);

    Layout layout;
    layout.edge = edge;
    layout.verticalEdge = edge == Edge::Left || edge == Edge::Right;

    // The stage spans the whole edge so position changes never move the layer surface.
    if (layout.verticalEdge) {
        layout.stageSize = QSize(StageWidth, available.height());
    } else {
        layout.stageSize = QSize(available.width(), StageHeight);
    }
    layout.cardSize = QSize(std::max(1, std::min(StageWidth, layout.stageSize.width()) - 2 * CardMargin),
                            std::max(1, std::min(StageHeight, layout.stageSize.height()) - 2 * CardMargin));

    const int edgeAxis = layout.verticalEdge ? layout.stageSize.height() : layout.stageSize.width();
    const int cardAxis = layout.verticalEdge ? layout.cardSize.height() : layout.cardSize.width();
    const int axisMax = std::max(0, edgeAxis - cardAxis - 2 * CardMargin);
    const int alongAxis = CardMargin + qRound(axisMax * (percent / 100.0));

    int x = 0;
    int y = 0;
    switch (edge) {
    case Edge::Left:
        x = CardMargin;
        y = alongAxis;
        break;
    case Edge::Right:
        x = layout.stageSize.width() - layout.cardSize.width() - CardMargin;
        y = alongAxis;
        break;
    case Edge::Top:
        x = alongAxis;
        y = CardMargin;
        break;
    case Edge::Bottom:
        x = alongAxis;
        y = layout.stageSize.height() - layout.cardSize.height() - CardMargin;
        break;
    }
    x = clampInt(x, CardMargin, layout.stageSize.width() - layout.cardSize.width() - CardMargin);
    y = clampInt(y, CardMargin, layout.stageSize.height() - layout.cardSize.height() - CardMargin);
    layout.cardRect = QRect(x, y, layout.cardSize.width(), layout.cardSize.height());

    // Margins are measured from the screen edge, so panel offsets keep the strip in the usable area.
    const int panelLeft = available.left() - screenGeometry.left();
    const int panelTop = available.top() - screenGeometry.top();
    const int panelRight = screenGeometry.right() - available.right();
    const int panelBottom = screenGeometry.bottom() - available.bottom();
    layout.margins = QMargins(panelLeft, panelTop, panelRight, panelBottom);

    if (layout.verticalEdge) {
        const int handleY = layout.cardRect.y() + (layout.cardSize.height() - HandleLength) / 2;
        const int handleX = edge == Edge::Right ? layout.stageSize.width() - HandleThickness : 0;
        const int clampedY = clampInt(handleY, 0, layout.stageSize.height() - HandleLength);
        layout.handleRect = QRect(handleX, clampedY, HandleThickness, HandleLength);
        const int hitHeight = std::min(HandleInputLength, layout.stageSize.height());
        const int hitWidth = std::min(HandleInputThickness, layout.stageSize.width());
        const int hitX = edge == Edge::Right ? layout.stageSize.width() - hitWidth : 0;
        const int hitY = clampInt(handleY, 0, layout.stageSize.height() - hitHeight);
        layout.handleHitRect = QRect(hitX, hitY, hitWidth, hitHeight);
    } else {
        const int handleY = edge == Edge::Bottom ? layout.stageSize.height() - HandleThickness : 0;
        const int handleX = layout.cardRect.x() + (layout.cardSize.width() - HandleLength) / 2;
        const int clampedX = clampInt(handleX, 0, layout.stageSize.width() - HandleLength);
        layout.handleRect = QRect(clampedX, handleY, HandleLength, HandleThickness);
        const int hitWidth = std::min(HandleInputLength, layout.stageSize.width());
        const int hitHeight = std::min(HandleInputThickness, layout.stageSize.height());
        const int hitY = edge == Edge::Bottom ? layout.stageSize.height() - hitHeight : 0;
        const int hitX = clampInt(handleX, 0, layout.stageSize.width() - hitWidth);
        layout.handleHitRect = QRect(hitX, hitY, hitWidth, hitHeight);
    }

    return layout;
}

QRegion ShelfGeometry::collapsedInputRegion(const Layout &layout)
{
    return QRegion(layout.handleHitRect);
}

QRegion ShelfGeometry::expandedInputRegion(const Layout &layout)
{
    return QRegion(layout.cardRect.united(layout.handleRect));
}
