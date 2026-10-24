/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls

AbstractButton {
    id: control

    property string iconName: "text-x-generic"
    property string accessibleName: ""
    property int iconSize: 18
    property bool accented: false
    property bool danger: false

    readonly property var theme: shelf.theme

    implicitWidth: 30
    implicitHeight: 30
    padding: 0
    hoverEnabled: true
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: control.accessibleName.length > 0 ? control.accessibleName : control.iconName

    background: Rectangle {
        radius: 8
        color: control.down
               ? control.theme.pressed
               : control.danger && control.hovered
                 ? control.theme.dangerSoft
                 : control.accented
                   ? control.theme.accentSoft
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

    contentItem: Image {
        source: "image://yoink/icon/" + encodeURIComponent("theme:" + control.iconName)
        sourceSize: Qt.size(control.iconSize * 3, control.iconSize * 3)
        anchors.centerIn: parent
        smooth: true
        opacity: control.enabled ? 1 : 0.5
    }
}
