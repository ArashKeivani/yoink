/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

FocusScope {
    id: root

    anchors.fill: parent
    visible: shelf.menuOpen
    focus: visible
    z: 30

    readonly property var theme: shelf.theme

    function close() {
        shelf.closeMenu();
    }

    Rectangle {
        anchors.fill: parent
        radius: 16
        color: root.theme.scrim
        opacity: 0.6

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    Rectangle {
        id: panel
        width: 232
        height: menuColumn.implicitHeight + 16
        x: parent.width - width - 8
        y: 58
        radius: 12
        color: root.theme.menu
        border.width: 1
        border.color: root.theme.border

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: menuColumn
            x: 8
            y: 8
            width: panel.width - 16
            spacing: 2

            MenuRow {
                width: menuColumn.width
                label: qsTr("Settings")
                iconName: "configure"
                onClicked: {
                    root.close();
                    shelf.openSettings();
                }
            }

            MenuRow {
                width: menuColumn.width
                label: qsTr("Keep Shelf Open")
                iconName: "pin"
                marked: shelf.settings.pinned
                onClicked: {
                    shelf.togglePinned();
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                label: qsTr("Start at Login")
                iconName: "application-x-executable"
                marked: shelf.autostartEnabled
                onClicked: {
                    shelf.setAutostartEnabled(!shelf.autostartEnabled);
                    root.close();
                }
            }

            Rectangle {
                width: menuColumn.width
                height: 1
                color: root.theme.divider
            }

            MenuRow {
                width: menuColumn.width
                visible: shelf.itemCount > 0
                label: qsTr("Clear All")
                iconName: "edit-clear"
                danger: true
                onClicked: {
                    shelf.clearShelf();
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                label: qsTr("Quit Yoink")
                iconName: "application-exit"
                onClicked: shelf.quit()
            }
        }
    }
}
