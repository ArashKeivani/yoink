/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls

AbstractButton {
    id: control

    property string label: ""
    property bool selected: false

    readonly property var theme: shelf.theme

    implicitHeight: 28
    leftPadding: 12
    rightPadding: 12
    hoverEnabled: true
    activeFocusOnTab: true
    Accessible.role: Accessible.RadioButton
    Accessible.name: control.label

    background: Rectangle {
        radius: 14
        color: control.selected
               ? control.theme.accent
               : control.hovered
                 ? control.theme.hover
                 : "transparent"
        border.width: control.visualFocus ? 2 : 1
        border.color: control.selected ? control.theme.accent : control.theme.border
        opacity: control.enabled ? 1 : 0.5
        Behavior on color {
            ColorAnimation {
                duration: control.theme.duration(120)
            }
        }
    }

    contentItem: Text {
        text: control.label
        color: control.selected ? control.theme.accentText : control.theme.text
        font.family: control.theme.fontFamily
        font.pointSize: control.theme.smallPointSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
