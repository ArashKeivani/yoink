/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls

AbstractButton {
    id: control

    property bool danger: false

    readonly property var theme: shelf.theme

    implicitHeight: 28
    leftPadding: 9
    rightPadding: 9
    hoverEnabled: true
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: control.text

    background: Rectangle {
        radius: 7
        color: control.down
               ? control.theme.pressed
               : control.danger && control.hovered
                 ? control.theme.dangerSoft
                 : control.hovered
                   ? control.theme.hover
                   : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: control.theme.accent
        opacity: control.enabled ? 1 : 0.5
        Behavior on color {
            ColorAnimation {
                duration: control.theme.duration(120)
            }
        }
    }

    contentItem: Text {
        text: control.text
        color: control.danger ? control.theme.danger : control.theme.text
        font.family: control.theme.fontFamily
        font.pointSize: control.theme.bodyPointSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        opacity: control.enabled ? 1 : 0.5
    }
}
