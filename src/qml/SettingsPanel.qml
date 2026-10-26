/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    anchors.fill: parent
    visible: shelf.settingsOpen
    focus: visible
    z: 40

    readonly property var theme: shelf.theme

    function formatSeconds(ms) {
        const seconds = ms / 1000;
        return Number.isInteger(seconds) ? seconds + " s" : seconds.toFixed(1) + " s";
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
        height: 48
        color: root.theme.header
        topLeftRadius: 16
        topRightRadius: 16

        IconButton {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            iconName: "go-previous"
            accessibleName: qsTr("Back")
            onClicked: shelf.closeSettings()
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 46
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Settings")
            color: root.theme.text
            font.family: root.theme.fontFamily
            font.pointSize: root.theme.titlePointSize
            font.weight: Font.DemiBold
        }

        IconButton {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            iconName: "window-close"
            accessibleName: qsTr("Close settings")
            onClicked: shelf.closeSettings()
        }
    }

    Rectangle {
        id: headerDivider
        anchors.top: header.bottom
        width: parent.width
        height: 1
        color: root.theme.divider
    }

    Flickable {
        id: flickable
        anchors.top: headerDivider.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight + 28
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick

        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            policy: ScrollBar.AsNeeded
            width: 6

            contentItem: Rectangle {
                implicitWidth: 4
                implicitHeight: 36
                radius: 2
                color: root.theme.scrollbar
                opacity: scrollBar.pressed ? 0.9 : 0.55
            }

            background: Rectangle {
                color: "transparent"
            }
        }

        ColumnLayout {
            id: content
            x: 16
            y: 14
            width: flickable.width - 32
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                visible: shelf.readOnly
                height: readOnlyText.implicitHeight + 16
                radius: 8
                color: root.theme.dangerSoft
                border.width: 1
                border.color: root.theme.danger

                Text {
                    id: readOnlyText
                    anchors.fill: parent
                    anchors.margins: 8
                    text: qsTr("This shelf is read-only because its data was created by a newer version of Yoink.")
                    color: root.theme.danger
                    font.family: root.theme.fontFamily
                    font.pointSize: root.theme.smallPointSize
                    wrapMode: Text.WordWrap
                }
            }

            Text {
                text: qsTr("EDGE")
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pointSize: root.theme.smallPointSize
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
            }

            Flow {
                Layout.fillWidth: true
                spacing: 6

                Repeater {
                    model: [qsTr("Left"), qsTr("Right"), qsTr("Top"), qsTr("Bottom")]

                    delegate: OptionChip {
                        label: modelData
                        selected: shelf.settings.edge === index
                        onClicked: shelf.setEdge(index)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 2
                spacing: 8

                Text {
                    text: qsTr("POSITION")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pointSize: root.theme.smallPointSize
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                }

                Item {
                    Layout.fillWidth: true
                }

                Text {
                    text: Math.round(positionSlider.value) + "%"
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pointSize: root.theme.smallPointSize
                }
            }

            Slider {
                id: positionSlider
                property int committed: shelf.settings.position

                onCommittedChanged: {
                    if (!pressed) {
                        value = committed;
                    }
                }
                Component.onCompleted: value = committed
                Connections {
                    target: shelf
                    function onLayoutChanged() {
                        positionSlider.commitTimer.stop();
                        positionSlider.value = shelf.settings.position;
                    }
                }
                Layout.fillWidth: true
                implicitHeight: 28
                from: 0
                to: 100
                stepSize: 1
                Accessible.name: qsTr("Shelf position")
                onMoved: {
                    if (shelf.verticalEdge) {
                        shelf.setPosition(Math.round(value));
                    } else {
                        commitTimer.restart();
                    }
                }
                // On top/bottom the card slides sideways as the position
                // changes, so applying live moves the target under the pointer.
                // The change lands when the drag is released instead.
                onPressedChanged: {
                    if (!pressed && !shelf.verticalEdge) {
                        commitTimer.stop();
                        shelf.setPosition(Math.round(value));
                    }
                }

                Timer {
                    id: commitTimer
                    interval: 350
                    onTriggered: {
                        if (positionSlider.pressed) {
                            restart();
                        } else {
                            shelf.setPosition(Math.round(positionSlider.value));
                        }
                    }
                }

                background: Rectangle {
                    x: positionSlider.leftPadding
                    y: positionSlider.topPadding + positionSlider.availableHeight / 2 - height / 2
                    width: positionSlider.availableWidth
                    height: 4
                    radius: 2
                    color: root.theme.divider

                    Rectangle {
                        width: positionSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 2
                        color: root.theme.accent
                    }
                }

                handle: Rectangle {
                    x: positionSlider.leftPadding + positionSlider.visualPosition * (positionSlider.availableWidth - width)
                    y: positionSlider.topPadding + positionSlider.availableHeight / 2 - height / 2
                    width: 16
                    height: 16
                    radius: 8
                    color: root.theme.accent
                    border.width: 1
                    border.color: root.theme.surface
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                spacing: 8

                Text {
                    text: qsTr("NOTIFICATION")
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pointSize: root.theme.smallPointSize
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                }

                Item {
                    Layout.fillWidth: true
                }

                Text {
                    text: root.formatSeconds(shelf.settings.toastDuration)
                    color: root.theme.textMuted
                    font.family: root.theme.fontFamily
                    font.pointSize: root.theme.smallPointSize
                }
            }

            Slider {
                id: toastSlider
                property int committed: shelf.settings.toastDuration

                onCommittedChanged: {
                    if (!pressed) {
                        value = committed / 1000;
                    }
                }
                Component.onCompleted: value = committed / 1000
                Layout.fillWidth: true
                implicitHeight: 28
                from: 1
                to: 8
                stepSize: 0.5
                Accessible.name: qsTr("Notification timeout")
                onMoved: shelf.settings.toastDuration = Math.round(value * 1000)

                background: Rectangle {
                    x: toastSlider.leftPadding
                    y: toastSlider.topPadding + toastSlider.availableHeight / 2 - height / 2
                    width: toastSlider.availableWidth
                    height: 4
                    radius: 2
                    color: root.theme.divider

                    Rectangle {
                        width: toastSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 2
                        color: root.theme.accent
                    }
                }

                handle: Rectangle {
                    x: toastSlider.leftPadding + toastSlider.visualPosition * (toastSlider.availableWidth - width)
                    y: toastSlider.topPadding + toastSlider.availableHeight / 2 - height / 2
                    width: 16
                    height: 16
                    radius: 8
                    color: root.theme.accent
                    border.width: 1
                    border.color: root.theme.surface
                }
            }

            Text {
                text: qsTr("How long notifications stay on screen. Errors stay at least four seconds.")
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pointSize: root.theme.smallPointSize
            }

            Text {
                Layout.topMargin: 2
                text: qsTr("MONITOR")
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pointSize: root.theme.smallPointSize
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
            }

            Flow {
                Layout.fillWidth: true
                spacing: 6

                Repeater {
                    model: shelf.screenNames

                    delegate: OptionChip {
                        label: modelData
                        selected: shelf.settings.screenName === modelData
                        onClicked: shelf.selectScreen(modelData)
                    }
                }
            }

            Text {
                visible: shelf.screenNames.length === 0
                text: qsTr("No screens detected.")
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pointSize: root.theme.smallPointSize
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: root.theme.divider
            }

            ToggleRow {
                Layout.fillWidth: true
                label: qsTr("Start at Login")
                description: qsTr("Start Yoink quietly when you sign in.")
                checked: shelf.autostartEnabled
                onToggled: function (checked) {
                    shelf.setAutostartEnabled(checked);
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: root.theme.divider
            }

            Text {
                text: qsTr("STORAGE")
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pointSize: root.theme.smallPointSize
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
            }

            Text {
                Layout.fillWidth: true
                text: shelf.storageLocation
                color: root.theme.textMuted
                font.family: root.theme.fontFamily
                font.pointSize: root.theme.smallPointSize
                elide: Text.ElideMiddle
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 2
                text: qsTr("Yoink %1 · Settings are saved automatically.").arg(shelf.version)
                color: root.theme.textMuted
                opacity: 0.8
                font.family: root.theme.fontFamily
                font.pointSize: root.theme.smallPointSize
                wrapMode: Text.WordWrap
            }
        }
    }
}
