/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Yoink 1.0

DropSurface {
    id: root

    width: shelf.stageWidth
    height: shelf.stageHeight
    focus: false

    readonly property var theme: shelf.theme

    Shortcut {
        sequence: "Escape"
        context: Qt.WindowShortcut
        onActivated: shelf.handleEscape()
    }

    Shortcut {
        sequence: "Ctrl+,"
        context: Qt.WindowShortcut
        enabled: shelf.expanded
        onActivated: shelf.openSettings()
    }

    FocusScope {
        id: focusScope
        anchors.fill: parent
        focus: !shelf.overlayOpen

        Item {
            id: handleArea
            x: shelf.handleHitRect.x
            y: shelf.handleHitRect.y
            width: shelf.handleHitRect.width
            height: shelf.handleHitRect.height
            z: 5

            Rectangle {
                id: handleVisual
                readonly property bool vertical: shelf.verticalEdge
                readonly property bool hovered: handleMouse.containsMouse

                x: vertical
                   ? (shelf.edge === 0 ? 0 : handleArea.width - width)
                   : (handleArea.width - width) / 2
                y: vertical
                   ? (handleArea.height - height) / 2
                   : (shelf.edge === 2 ? 0 : handleArea.height - height)
                width: vertical ? (hovered ? 10 : 8) : shelf.handleRect.width
                height: vertical ? shelf.handleRect.height : (hovered ? 10 : 8)
                radius: Math.min(width, height) / 2
                color: root.theme.accent
                opacity: hovered ? 0.95 : shelf.expanded ? 0.7 : 0.5

                Behavior on width {
                    NumberAnimation {
                        duration: root.theme.duration(140)
                        easing.type: Easing.OutCubic
                    }
                }
                Behavior on height {
                    NumberAnimation {
                        duration: root.theme.duration(140)
                        easing.type: Easing.OutCubic
                    }
                }
                Behavior on color {
                    ColorAnimation {
                        duration: root.theme.duration(140)
                    }
                }
                Behavior on opacity {
                    NumberAnimation {
                        duration: root.theme.duration(140)
                    }
                }
            }

            MouseArea {
                id: handleMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Yoink shelf handle")
                onClicked: shelf.expanded ? shelf.closeShelf() : shelf.showFromUser()
            }
        }

        Item {
            id: card

            readonly property real baseX: shelf.cardRect.x
            readonly property real baseY: shelf.cardRect.y

            function collapsedOffsetX() {
                if (shelf.edge === 0) {
                    return -width * 0.45;
                }
                if (shelf.edge === 1) {
                    return width * 0.45;
                }
                return 0;
            }

            function collapsedOffsetY() {
                if (shelf.edge === 2) {
                    return -height * 0.35;
                }
                if (shelf.edge === 3) {
                    return height * 0.35;
                }
                return 0;
            }

            x: baseX + (shelf.expanded ? 0 : collapsedOffsetX())
            y: baseY + (shelf.expanded ? 0 : collapsedOffsetY())
            width: shelf.cardWidth
            height: shelf.cardHeight
            opacity: shelf.expanded ? 1 : 0
            scale: shelf.expanded ? 1 : 0.94
            visible: shelf.expanded || opacity > 0.01
            enabled: shelf.expanded
            transformOrigin: Item.Center
            z: 10

            Behavior on x {
                NumberAnimation {
                    duration: root.theme.duration(220)
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on y {
                NumberAnimation {
                    duration: root.theme.duration(220)
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: root.theme.duration(200)
                }
            }
            Behavior on scale {
                NumberAnimation {
                    duration: root.theme.duration(220)
                    easing.type: Easing.OutCubic
                }
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                anchors.topMargin: 1
                anchors.bottomMargin: -3
                radius: 18
                color: root.theme.shadow
                opacity: 0.55
                z: -1
            }

            Rectangle {
                anchors.fill: parent
                radius: 16
                color: root.theme.surface
                border.width: 1
                border.color: root.theme.border
            }

            Rectangle {
                id: header
                width: parent.width
                height: 54
                color: root.theme.header
                topLeftRadius: 16
                topRightRadius: 16

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    Text {
                        text: qsTr("Yoink")
                        color: root.theme.text
                        font.family: root.theme.fontFamily
                        font.pointSize: root.theme.titlePointSize
                        font.weight: Font.DemiBold
                    }

                    Text {
                        text: shelf.itemCount === 0
                              ? qsTr("Empty")
                              : shelf.itemCount === 1
                                ? qsTr("1 item")
                                : qsTr("%1 items").arg(shelf.itemCount)
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pointSize: root.theme.smallPointSize
                    }
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    IconButton {
                        iconName: "pin"
                        accented: shelf.settings.pinned
                        accessibleName: shelf.settings.pinned ? qsTr("Stop keeping the shelf open") : qsTr("Keep the shelf open")
                        onClicked: shelf.togglePinned()
                    }

                    IconButton {
                        iconName: "configure"
                        accessibleName: qsTr("Settings")
                        onClicked: shelf.openSettings()
                    }

                    IconButton {
                        iconName: "window-close"
                        accessibleName: qsTr("Close shelf")
                        onClicked: shelf.closeShelf()
                    }
                }
            }

            Rectangle {
                id: headerDivider
                anchors.top: header.bottom
                width: parent.width
                height: 1
                color: root.theme.divider
            }

            Item {
                id: body
                anchors.top: headerDivider.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: footer.top

                EmptyState {
                    anchors.fill: parent
                    visible: shelf.itemCount === 0
                }

                ListView {
                    id: list
                    anchors.fill: parent
                    anchors.margins: 6
                    visible: shelf.itemCount > 0
                    clip: true
                    model: shelf.model
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.VerticalFlick
                    cacheBuffer: 240

                    add: Transition {
                        NumberAnimation {
                            property: "opacity"
                            from: 0
                            to: 1
                            duration: root.theme.duration(160)
                        }
                    }
                    remove: Transition {
                        NumberAnimation {
                            property: "opacity"
                            to: 0
                            duration: root.theme.duration(120)
                        }
                    }
                    displaced: Transition {
                        NumberAnimation {
                            properties: "y"
                            duration: root.theme.duration(160)
                            easing.type: Easing.OutCubic
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        id: listScrollBar
                        policy: ScrollBar.AsNeeded
                        width: 6

                        contentItem: Rectangle {
                            implicitWidth: 4
                            implicitHeight: 36
                            radius: 2
                            color: root.theme.scrollbar
                            opacity: listScrollBar.pressed ? 0.9 : 0.55
                        }

                        background: Rectangle {
                            color: "transparent"
                        }
                    }

                    delegate: ItemRow {
                        itemIndex: index
                        itemName: name
                        itemDetail: detail
                        itemDescription: description
                        itemIcon: iconSource
                        itemPreview: previewSource
                        itemAvailable: available
                        itemHasPreview: hasPreview
                        itemKindName: kindName
                    }
                }
            }

            Rectangle {
                id: footer
                anchors.bottom: parent.bottom
                width: parent.width
                height: 38
                color: "transparent"

                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: 1
                    color: root.theme.divider
                }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.right: clearAll.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: shelf.itemCount > 0 ? qsTr("Drag items out to use them again") : qsTr("Drop anything here")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pointSize: root.theme.smallPointSize
                    elide: Text.ElideRight
                }

                TextButton {
                    id: clearAll
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Clear all")
                    danger: true
                    visible: shelf.itemCount > 0
                    onClicked: shelf.clearShelf()
                }
            }

            SettingsPanel {
                anchors.fill: parent
            }

            ShelfMenu {
                anchors.fill: parent
            }

            ItemMenu {
                anchors.fill: parent
            }

            Rectangle {
                anchors.fill: parent
                radius: 16
                color: root.theme.accentSoft
                opacity: 0.97
                border.width: 2
                border.color: root.theme.accent
                visible: shelf.externalDragActive
                z: 90

                Column {
                    anchors.centerIn: parent
                    width: parent.width - 56
                    spacing: 9

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 64
                        height: 64
                        radius: 32
                        color: root.theme.surface

                        Image {
                            anchors.centerIn: parent
                            source: "image://yoink/icon/" + encodeURIComponent("theme:document-save")
                            sourceSize: Qt.size(90, 90)
                            width: 30
                            height: 30
                            smooth: true
                        }
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Drop to collect")
                        color: root.theme.text
                        font.family: root.theme.fontFamily
                        font.pointSize: root.theme.titlePointSize
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        width: parent.width
                        text: shelf.externalDragSummary.length > 0
                              ? shelf.externalDragSummary
                              : qsTr("Files, folders, text, links, and images")
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pointSize: root.theme.smallPointSize
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Toast {
                z: 100
            }
        }
    }
}
