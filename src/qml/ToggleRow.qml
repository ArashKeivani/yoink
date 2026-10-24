/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string label: ""
    property string description: ""
    property bool checked: false

    readonly property var theme: shelf.theme

    signal toggled(bool checked)

    implicitHeight: description.length > 0 ? 52 : 42
    activeFocusOnTab: true
    Accessible.role: Accessible.CheckBox
    Accessible.name: root.label
    Accessible.description: root.description

    Keys.onSpacePressed: root.toggled(!root.checked)
    Keys.onReturnPressed: root.toggled(!root.checked)
    Keys.onEnterPressed: root.toggled(!root.checked)

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: root.activeFocus ? root.theme.hover : "transparent"
        border.width: root.activeFocus ? 1 : 0
        border.color: root.theme.accent
        Behavior on color {
            ColorAnimation {
                duration: root.theme.duration(120)
            }
        }
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: track.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 1

        Text {
            width: parent.width
            text: root.label
            color: root.theme.text
            font.family: root.theme.fontFamily
            font.pointSize: root.theme.bodyPointSize
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            visible: root.description.length > 0
            text: root.description
            color: root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pointSize: root.theme.smallPointSize
            elide: Text.ElideRight
        }
    }

    Rectangle {
        id: track
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        width: 36
        height: 20
        radius: 10
        color: root.checked ? root.theme.accent : root.theme.divider
        Behavior on color {
            ColorAnimation {
                duration: root.theme.duration(140)
            }
        }

        Rectangle {
            width: 16
            height: 16
            radius: 8
            y: 2
            x: root.checked ? 18 : 2
            color: root.checked ? root.theme.accentText : root.theme.textMuted
            Behavior on x {
                NumberAnimation {
                    duration: root.theme.duration(160)
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.forceActiveFocus();
            root.toggled(!root.checked);
        }
    }
}
