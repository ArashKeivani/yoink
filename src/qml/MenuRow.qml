/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AbstractButton {
    id: control

    property string label: ""
    property string iconName: ""
    property bool marked: false
    property bool danger: false

    readonly property var theme: shelf.theme

    implicitHeight: 34
    leftPadding: 10
    rightPadding: 10
    hoverEnabled: true
    activeFocusOnTab: true
    Accessible.role: Accessible.MenuItem
    Accessible.name: control.label

    background: Rectangle {
        radius: 8
        color: control.down
               ? control.theme.pressed
               : control.hovered
                 ? control.theme.hover
                 : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: control.theme.accent
        Behavior on color {
            ColorAnimation {
                duration: control.theme.duration(110)
            }
        }
    }

    contentItem: RowLayout {
        width: control.availableWidth
        height: control.availableHeight
        spacing: 10

        Image {
            visible: control.iconName.length > 0
            source: "image://yoink/icon/" + encodeURIComponent("theme:" + control.iconName)
            sourceSize: Qt.size(48, 48)
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            smooth: true
        }

        Text {
            text: control.label
            Layout.fillWidth: true
            color: control.danger ? control.theme.danger : control.theme.text
            font.family: control.theme.fontFamily
            font.pointSize: control.theme.bodyPointSize
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Image {
            visible: control.marked
            source: "image://yoink/icon/" + encodeURIComponent("theme:checkmark")
            sourceSize: Qt.size(42, 42)
            Layout.preferredWidth: 14
            Layout.preferredHeight: 14
            smooth: true
        }
    }
}
