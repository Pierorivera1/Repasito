import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root
    width: parent ? parent.width : 400
    implicitHeight: mainLayout.implicitHeight + 20
    radius: 8
    color: isSelected ? backend.themeSelection : backend.themeLighterBg
    border.color: isSelected ? backend.themeAccent : (backend.darkMode ? "#24282c" : "#e5e7eb")
    border.width: 1

    property var itemData: null
    property bool isSelected: false
    property bool notesExpanded: false

    signal reviewToggled(int reviewId, bool completed)
    signal snoozeRequested(int reviewId)
    signal editNotesRequested(int topicId, string currentNotes)
    signal deleteRequested(int topicId, string topicTitle)
    signal clicked()

    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 8

        // Top Row: Checkbox, Title, Badges, Action Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Custom Checkbox
            Rectangle {
                width: 20
                height: 20
                radius: 4
                color: itemData && itemData.isCompleted ? backend.themeAccent : "transparent"
                border.color: itemData && itemData.isCompleted ? backend.themeAccent : (backend.darkMode ? "#555b62" : "#a0aec0")
                border.width: 1.5

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    font.pixelSize: 12
                    font.bold: true
                    color: backend.darkMode ? "#101315" : "#ffffff"
                    visible: itemData && itemData.isCompleted
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (itemData) {
                            root.reviewToggled(itemData.reviewId, !itemData.isCompleted);
                        }
                    }
                }
            }

            // Title & Review Stage
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: itemData ? itemData.title : ""
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        font.strikeout: itemData ? itemData.isCompleted : false
                        color: itemData && itemData.isCompleted ? backend.themeMuted : backend.themeForeground
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Stage Pill (R1, R2, R3)
                    Rectangle {
                        height: 18
                        width: stageText.implicitWidth + 10
                        radius: 4
                        color: backend.darkMode ? "#22262a" : "#e2e8f0"
                        border.color: backend.darkMode ? "#343a40" : "#cbd5e1"
                        border.width: 1

                        Text {
                            id: stageText
                            anchors.centerIn: parent
                            text: {
                                if (!itemData) return "";
                                switch(itemData.stage) {
                                    case 1: return "R1 (+1d)";
                                    case 2: return "R2 (+3d)";
                                    case 3: return "R3 (+5d)";
                                    default: return "R" + itemData.stage;
                                }
                            }
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 10
                            color: backend.themeMuted
                        }
                    }

                    // Overdue Badge
                    Rectangle {
                        visible: itemData ? itemData.isOverdue : false
                        height: 18
                        width: overdueText.implicitWidth + 10
                        radius: 4
                        color: backend.darkMode ? "#421818" : "#fee2e2"
                        border.color: backend.darkMode ? "#7f1d1d" : "#fca5a5"
                        border.width: 1

                        Text {
                            id: overdueText
                            anchors.centerIn: parent
                            text: "OVERDUE"
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 9
                            font.bold: true
                            color: backend.darkMode ? "#f87171" : "#dc2626"
                        }
                    }

                    // Date Pill
                    Text {
                        text: itemData ? itemData.formattedDate : ""
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 11
                        color: backend.themeMuted
                    }
                }
            }

            // Quick Actions (Snooze, Notes, Delete)
            RowLayout {
                spacing: 4

                // Snooze 1d
                Button {
                    text: "+1d"
                    flat: true
                    implicitHeight: 24
                    implicitWidth: 32
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 10
                    Material.foreground: backend.themeMuted
                    ToolTip.visible: hovered
                    ToolTip.text: "Snooze review 1 day"
                    onClicked: {
                        if (itemData) root.snoozeRequested(itemData.reviewId);
                    }
                }

                // Edit Notes Button
                Button {
                    text: "✎"
                    flat: true
                    implicitHeight: 24
                    implicitWidth: 26
                    font.pixelSize: 12
                    Material.foreground: backend.themeMuted
                    ToolTip.visible: hovered
                    ToolTip.text: "View / edit notes"
                    onClicked: {
                        if (itemData) root.editNotesRequested(itemData.topicId, itemData.notes);
                    }
                }

                // Delete Topic Button
                Button {
                    text: "✕"
                    flat: true
                    implicitHeight: 24
                    implicitWidth: 24
                    font.pixelSize: 11
                    Material.foreground: backend.themeMuted
                    ToolTip.visible: hovered
                    ToolTip.text: "Delete topic"
                    onClicked: {
                        if (itemData) root.deleteRequested(itemData.topicId, itemData.title || "");
                    }
                }
            }
        }

        // Optional Notes Section / Preview
        ColumnLayout {
            Layout.fillWidth: true
            visible: itemData && itemData.notes && itemData.notes.trim().length > 0
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: backend.darkMode ? "#22262a" : "#edf2f7"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: root.notesExpanded ? "▼ Notes" : "▶ Notes"
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 11
                    color: backend.themeMuted
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.notesExpanded = !root.notesExpanded
                    }
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                visible: root.notesExpanded
                Layout.fillWidth: true
                radius: 4
                color: backend.darkMode ? "#121517" : "#f8f9fa"
                border.color: backend.darkMode ? "#24282c" : "#e2e8f0"
                border.width: 1
                implicitHeight: notesText.implicitHeight + 16

                Text {
                    id: notesText
                    anchors.fill: parent
                    anchors.margins: 8
                    text: itemData ? itemData.notes : ""
                    textFormat: Text.MarkdownText
                    wrapMode: Text.WordWrap
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 12
                    color: backend.themeForeground
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        z: -1
        onClicked: root.clicked()
    }
}
