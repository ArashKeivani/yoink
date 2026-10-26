/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

FocusScope {
    id: root

    anchors.fill: parent
    visible: shelf.itemMenuOpen
    focus: visible
    z: 30

    readonly property var theme: shelf.theme
    property int itemIndex: shelf.itemMenuIndex
    property string kindName: itemIndex >= 0 ? shelf.itemData(itemIndex, "kindName") : ""
    property string itemName: itemIndex >= 0 ? shelf.itemData(itemIndex, "name") : ""
    property string itemDetail: itemIndex >= 0 ? shelf.itemData(itemIndex, "detail") : ""
    property string itemIcon: itemIndex >= 0 ? shelf.itemData(itemIndex, "iconSource") : ""
    property bool multiMode: shelf.selectedCount > 1 && shelf.selectedRows.indexOf(root.itemIndex) >= 0

    function close() {
        shelf.closeItemMenu();
    }

    function selectionContainsOnly(kinds) {
        const rows = shelf.selectedRows;
        if (rows.length === 0) {
            return false;
        }
        for (let i = 0; i < rows.length; ++i) {
            if (kinds.indexOf(shelf.itemData(rows[i], "kindName")) < 0) {
                return false;
            }
        }
        return true;
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
        width: 238
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

            Item {
                width: menuColumn.width
                height: 42
                visible: !root.multiMode

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 9

                    Image {
                        source: root.itemIcon
                        sourceSize: Qt.size(84, 84)
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        smooth: true
                    }

                    Column {
                        Layout.fillWidth: true
                        spacing: 1

                        Text {
                            width: parent.width
                            text: root.itemName
                            color: root.theme.menuText
                            font.family: root.theme.fontFamily
                            font.pointSize: root.theme.bodyPointSize
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            text: root.itemDetail
                            color: root.theme.textMuted
                            font.family: root.theme.fontFamily
                            font.pointSize: root.theme.smallPointSize
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Item {
                width: menuColumn.width
                height: 42
                visible: root.multiMode

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    Text {
                        width: parent.width
                        text: qsTr("%n item(s) selected", "", shelf.selectedCount)
                        color: root.theme.menuText
                        font.family: root.theme.fontFamily
                        font.pointSize: root.theme.bodyPointSize
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Group actions")
                        color: root.theme.textMuted
                        font.family: root.theme.fontFamily
                        font.pointSize: root.theme.smallPointSize
                        elide: Text.ElideRight
                    }
                }
            }

            Rectangle {
                width: menuColumn.width
                height: 1
                color: root.theme.divider
            }

            MenuRow {
                width: menuColumn.width
                visible: root.multiMode && root.selectionContainsOnly(["file", "folder", "image", "url"])
                label: qsTr("Open %n item(s)", "", shelf.selectedCount)
                iconName: "document-open"
                onClicked: {
                    shelf.openSelectedItems();
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                visible: root.multiMode && root.selectionContainsOnly(["file", "folder", "image"])
                label: qsTr("Show in Folder")
                iconName: "folder-open"
                onClicked: {
                    shelf.showSelectedInFolder();
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                visible: root.multiMode
                label: qsTr("Copy %n item(s)", "", shelf.selectedCount)
                iconName: "edit-copy"
                onClicked: {
                    shelf.copySelectedItems();
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                visible: !root.multiMode && root.kindName === "text"
                label: qsTr("Copy Text")
                iconName: "edit-copy"
                onClicked: {
                    shelf.copyItem(root.itemIndex);
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                visible: !root.multiMode && root.kindName === "url"
                label: qsTr("Open Link")
                iconName: "internet-web-browser"
                onClicked: {
                    shelf.activateItem(root.itemIndex);
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                visible: !root.multiMode && root.kindName === "url"
                label: qsTr("Copy Link")
                iconName: "edit-copy"
                onClicked: {
                    shelf.copyItem(root.itemIndex);
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                visible: !root.multiMode && (root.kindName === "file" || root.kindName === "folder" || root.kindName === "image")
                label: root.kindName === "image" ? qsTr("Open Image") : qsTr("Open")
                iconName: "document-open"
                onClicked: {
                    shelf.activateItem(root.itemIndex);
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                visible: !root.multiMode && (root.kindName === "file" || root.kindName === "folder" || root.kindName === "image")
                label: root.kindName === "image" ? qsTr("Copy Image") : qsTr("Copy")
                iconName: "edit-copy"
                onClicked: {
                    shelf.copyItem(root.itemIndex);
                    root.close();
                }
            }

            MenuRow {
                width: menuColumn.width
                visible: !root.multiMode && (root.kindName === "file" || root.kindName === "folder" || root.kindName === "image")
                label: qsTr("Show in Folder")
                iconName: "folder-open"
                onClicked: {
                    shelf.openContainingFolder(root.itemIndex);
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
                label: root.multiMode ? qsTr("Remove %n item(s)", "", shelf.selectedCount) : qsTr("Remove from Shelf")
                iconName: "user-trash"
                danger: true
                onClicked: {
                    if (root.multiMode) {
                        shelf.removeSelectedItems();
                    } else {
                        shelf.removeItem(root.itemIndex);
                    }
                    root.close();
                }
            }
        }
    }
}
