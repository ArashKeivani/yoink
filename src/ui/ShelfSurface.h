/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "core/ShelfPlacement.h"

#include <QQuickView>

namespace LayerShellQt
{
class Window;
}

class ShelfSurface : public QQuickView
{
    Q_OBJECT

public:
    explicit ShelfSurface(QWindow *parent = nullptr);

    [[nodiscard]] bool initializeLayerShell();
    void applyLayout(const ShelfGeometry::Layout &layout, QScreen *screen);
    void setInputExpanded(bool expanded);
    [[nodiscard]] bool inputExpanded() const;
    [[nodiscard]] const ShelfGeometry::Layout &layout() const;
    [[nodiscard]] QScreen *targetScreen() const;

Q_SIGNALS:
    void pointerInsideChanged(bool inside);

protected:
    bool event(QEvent *event) override;

private:
    void updateInputRegion();

    LayerShellQt::Window *m_layer = nullptr;
    ShelfGeometry::Layout m_layout;
    QScreen *m_targetScreen = nullptr;
    bool m_inputExpanded = true;
    bool m_pointerInside = false;
};
