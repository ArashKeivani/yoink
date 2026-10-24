/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

Item {
    id: root

    readonly property var theme: shelf.theme

    Column {
        anchors.centerIn: parent
        width: Math.min(root.width - 48, 260)
        spacing: 9

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 64
            height: 64
            radius: 32
            color: root.theme.accentSoft

            Image {
                anchors.centerIn: parent
                source: "image://yoink/icon/" + encodeURIComponent("theme:folder-drag-accept")
                sourceSize: Qt.size(96, 96)
                width: 32
                height: 32
                smooth: true
            }
        }

        Text {
            width: parent.width
            text: qsTr("Drop things here")
            color: root.theme.text
            font.family: root.theme.fontFamily
            font.pointSize: root.theme.bodyPointSize + 1
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            text: qsTr("Files, folders, text, links, and images stay on this shelf.")
            color: root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pointSize: root.theme.smallPointSize
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            lineHeight: 1.25
        }

        Text {
            width: parent.width
            text: qsTr("Drag items out to use them again.")
            color: root.theme.textMuted
            opacity: 0.75
            font.family: root.theme.fontFamily
            font.pointSize: root.theme.smallPointSize
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }

    Accessible.role: Accessible.StaticText
    Accessible.name: qsTr("Drop things here. Files, folders, text, links, and images stay on this shelf.")
}
