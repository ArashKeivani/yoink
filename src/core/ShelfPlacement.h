/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QMargins>
#include <QRect>
#include <QRegion>
#include <QSize>

namespace ShelfGeometry
{
enum class Edge {
    Left = 0,
    Right = 1,
    Top = 2,
    Bottom = 3,
};

struct Layout {
    Edge edge = Edge::Right;
    QSize stageSize;
    QSize cardSize;
    QMargins margins;
    QRect cardRect;
    QRect handleRect;
    QRect handleHitRect;
    bool verticalEdge = true;
};

[[nodiscard]] Layout computeLayout(const QRect &screenGeometry, const QRect &availableGeometry, Edge edge, int positionPercent);
[[nodiscard]] QRegion collapsedInputRegion(const Layout &layout);
[[nodiscard]] QRegion expandedInputRegion(const Layout &layout);
} // namespace ShelfGeometry
