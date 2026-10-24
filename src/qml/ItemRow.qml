/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls

Item {
    id: root

    property int itemIndex: -1
    property string itemName: ""
    property string itemDetail: ""
    property string itemDescription: ""
    property string itemIcon: ""
    property string itemPreview: ""
    property bool itemAvailable: true
    property bool itemHasPreview: false
    property string itemKindName: "file"
    property bool hovered: false

    readonly property var theme: shelf.theme

    width: ListView.view ? ListView.view.width : 0
    height: 62
    opacity: itemAvailable ? 1 : 0.62
    activeFocusOnTab: true

    Accessible.role: Accessible.ListItem
    Accessible.name: root.itemName
    Accessible.description: root.itemDescription

    Keys.onReturnPressed: shelf.activateItem(root.itemIndex)
    Keys.onEnterPressed: shelf.activateItem(root.itemIndex)
    Keys.onSpacePressed: shelf.activateItem(root.itemIndex)
    Keys.onDeletePressed: shelf.removeItem(root.itemIndex)
    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_Menu) {
            shelf.openItemMenu(root.itemIndex);
            event.accepted = true;
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: root.hovered || root.activeFocus ? root.theme.hover : "transparent"
        border.width: root.activeFocus ? 1 : 0
        border.color: root.theme.accent
        Behavior on color {
            ColorAnimation {
                duration: root.theme.duration(110)
            }
        }
    }

    Item {
        id: iconBox
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        width: 40
        height: 40

        Image {
            id: baseIcon
            anchors.centerIn: parent
            source: root.itemIcon
            sourceSize: Qt.size(102, 102)
            width: 34
            height: 34
            smooth: true
            visible: !preview.visible
        }

        Image {
            id: preview
            anchors.fill: parent
            source: root.itemPreview
            sourceSize: Qt.size(120, 120)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
            visible: root.itemHasPreview && root.itemAvailable && status === Image.Ready
        }
    }

    Column {
        anchors.left: iconBox.right
        anchors.leftMargin: 10
        anchors.right: removeButton.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        spacing: 1

        Text {
            width: parent.width
            text: root.itemName
            color: root.itemAvailable ? root.theme.text : root.theme.textMuted
            font.family: root.theme.fontFamily
            font.pointSize: root.theme.bodyPointSize
            font.weight: Font.Medium
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: root.itemDetail
            color: root.itemAvailable ? root.theme.textMuted : root.theme.danger
            font.family: root.theme.fontFamily
            font.pointSize: root.theme.smallPointSize
            elide: Text.ElideRight
        }
    }

    IconButton {
        id: removeButton
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.verticalCenter: parent.verticalCenter
        width: 26
        height: 26
        iconSize: 14
        iconName: "list-remove"
        accessibleName: qsTr("Remove %1").arg(root.itemName)
        visible: root.hovered || root.activeFocus || removeButton.hovered
        opacity: visible ? 1 : 0
        z: 2
        Behavior on opacity {
            NumberAnimation {
                duration: root.theme.duration(120)
            }
        }
        onClicked: shelf.removeItem(root.itemIndex)
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor

        property real pressX: 0
        property real pressY: 0
        property bool dragStarted: false

        onEntered: root.hovered = true
        onExited: root.hovered = false
        onPressed: function (mouse) {
            root.forceActiveFocus();
            pressX = mouse.x;
            pressY = mouse.y;
            dragStarted = false;
        }
        onPositionChanged: function (mouse) {
            if (!pressed || dragStarted) {
                return;
            }
            if (Math.abs(mouse.x - pressX) > 6 || Math.abs(mouse.y - pressY) > 6) {
                dragStarted = true;
                shelf.startItemDrag(root.itemIndex);
            }
        }
        onClicked: function (mouse) {
            if (dragStarted) {
                dragStarted = false;
                return;
            }
            if (mouse.button === Qt.RightButton) {
                shelf.openItemMenu(root.itemIndex);
            } else {
                shelf.activateItem(root.itemIndex);
            }
        }
        onPressAndHold: shelf.openItemMenu(root.itemIndex)
    }
}
