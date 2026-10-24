/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

Rectangle {
    id: toast

    readonly property var theme: shelf.theme

    width: parent.width - 24
    height: Math.max(42, toastText.implicitHeight + 20)
    x: 12
    y: parent.height - height - 14 + (shelf.toastVisible ? 0 : 8)
    radius: 10
    color: shelf.toastIsError ? theme.dangerSoft : theme.menu
    border.width: 1
    border.color: shelf.toastIsError ? theme.danger : theme.border
    opacity: shelf.toastVisible ? 1 : 0
    visible: shelf.toastVisible || opacity > 0.01
    z: 100

    Accessible.role: Accessible.AlertMessage
    Accessible.name: shelf.toastMessage

    Behavior on opacity {
        NumberAnimation {
            duration: toast.theme.duration(170)
        }
    }
    Behavior on y {
        NumberAnimation {
            duration: toast.theme.duration(180)
            easing.type: Easing.OutCubic
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Text {
            id: toastText
            Layout.fillWidth: true
            text: shelf.toastMessage
            color: shelf.toastIsError ? toast.theme.danger : toast.theme.menuText
            font.family: toast.theme.fontFamily
            font.pointSize: toast.theme.smallPointSize
            wrapMode: Text.WordWrap
            lineHeight: 1.2
            verticalAlignment: Text.AlignVCenter
        }

        TextButton {
            visible: shelf.toastUndoAvailable
            text: qsTr("Undo")
            onClicked: shelf.undoLastRemoval()
        }
    }
}
