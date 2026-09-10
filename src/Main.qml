import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: window
    title: "Omacalendar"
    width: 600
    height: 740
    minimumWidth: 440
    minimumHeight: 480
    visible: true
    color: backend.themeBackground

    Material.theme: backend.darkMode ? Material.Dark : Material.Light
    Material.accent: backend.themeAccent
    Material.background: backend.themeBackground
    Material.foreground: backend.themeForeground

    Component.onCompleted: {
        var geo = backend.windowGeometry();
        if (geo.valid) {
            window.x = geo.x;
            window.y = geo.y;
            window.width = geo.width;
            window.height = geo.height;
            if (geo.maximized) {
                window.showMaximized();
            }
        }
    }

    onClosing: {
        backend.saveWindowGeometry(window.x, window.y, window.width, window.height,
                                   window.visibility === Window.Maximized);
    }

    // Keyboard Shortcuts
    Shortcut {
        sequence: "Ctrl+N"
        onActivated: addTopicModal.openModal()
    }
    Shortcut {
        sequence: "N"
        enabled: !searchField.activeFocus
        onActivated: addTopicModal.openModal()
    }
    Shortcut {
        sequence: "/"
        enabled: !searchField.activeFocus
        onActivated: {
            searchField.forceActiveFocus();
            searchField.selectAll();
        }
    }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (searchField.activeFocus || searchField.text.length > 0) {
                searchField.text = "";
                agendaView.forceActiveFocus();
            }
        }
    }
    Shortcut {
        sequence: "J"
        enabled: !searchField.activeFocus
        onActivated: agendaView.incrementCurrentIndex()
    }
    Shortcut {
        sequence: "K"
        enabled: !searchField.activeFocus
        onActivated: agendaView.decrementCurrentIndex()
    }
    Shortcut {
        sequence: "Space"
        enabled: !searchField.activeFocus && agendaView.currentItem !== null
        onActivated: {
            var item = agendaView.model[agendaView.currentIndex];
            if (item) {
                backend.toggleReview(item.reviewId, !item.isCompleted);
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Top Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2

                Text {
                    text: "Omacalendar"
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 18
                    font.bold: true
                    color: backend.themeForeground
                }

                Text {
                    text: backend.todayDateString
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 11
                    color: backend.themeMuted
                }
            }

            Item { Layout.fillWidth: true }

            // Search Bar
            TextField {
                id: searchField
                placeholderText: "Search (/)"
                font.family: "iA Writer Mono S"
                font.pixelSize: 12
                color: backend.themeForeground
                Material.accent: backend.themeAccent
                implicitWidth: 160
                selectByMouse: true
                background: Rectangle {
                    color: backend.themeLighterBg
                    radius: 6
                    border.color: searchField.activeFocus ? backend.themeAccent : (backend.darkMode ? "#282c30" : "#e5e7eb")
                    border.width: 1
                }
                onTextChanged: backend.searchQuery = text
            }

            // Add Topic Button
            Button {
                id: addButton
                text: "+ Add Topic"
                highlighted: true
                font.family: "iA Writer Mono S"
                font.pixelSize: 12
                font.bold: true
                Material.background: backend.themeAccent
                Material.foreground: backend.themeAccentForeground
                contentItem: Text {
                    text: addButton.text
                    font: addButton.font
                    color: backend.themeAccentForeground
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                implicitHeight: 36
                ToolTip.visible: hovered
                ToolTip.text: "Schedule new spaced repetition (N)"
                onClicked: addTopicModal.openModal()
            }
        }

        // Mini-Calendar Strip (Week overview)
        DayStrip {
            id: dayStrip
            Layout.fillWidth: true
            onDateSelected: function(date) {
                if (date.length > 0) {
                    searchField.text = date;
                } else {
                    searchField.text = "";
                }
            }
        }

        // Agenda List
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"

            ListView {
                id: agendaView
                anchors.fill: parent
                clip: true
                spacing: 6
                model: backend.agenda
                focus: true

                // Section Headers
                section.property: "section"
                section.criteria: ViewSection.FullString
                section.delegate: Rectangle {
                    width: agendaView.width
                    height: 34
                    color: "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.topMargin: 8
                        anchors.bottomMargin: 4
                        spacing: 8

                        Text {
                            text: section
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            color: section === "Today" ? backend.themeAccent : backend.themeMuted
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: backend.darkMode ? "#24282c" : "#e5e7eb"
                        }
                    }
                }

                delegate: ReviewCard {
                    width: agendaView.width
                    itemData: modelData
                    isSelected: (agendaView.currentIndex === index)
                    onReviewToggled: function(reviewId, completed) {
                        backend.toggleReview(reviewId, completed);
                    }
                    onSnoozeRequested: function(reviewId) {
                        backend.snoozeReview(reviewId, 1);
                    }
                    onEditNotesRequested: function(topicId, notes) {
                        editNotesModal.openForTopic(topicId, notes);
                    }
                    onDeleteRequested: function(topicId) {
                        deleteConfirmDialog.topicIdToDelete = topicId;
                        deleteConfirmDialog.open();
                    }
                    onClicked: {
                        agendaView.currentIndex = index;
                    }
                }

                // Empty State
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: agendaView.count === 0
                    spacing: 12

                    Text {
                        text: "🎉"
                        font.pixelSize: 36
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: searchField.text.length > 0 ? "No reviews match your search." : "No reviews pending!"
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        color: backend.themeForeground
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Press 'N' or click '+ Add Topic' to schedule reviews."
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 12
                        color: backend.themeMuted
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }

        // Bottom status / tip bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Shortcuts: [N] Add topic   [/] Search   [J/K] Navigate   [Space] Complete"
                font.family: "iA Writer Mono S"
                font.pixelSize: 10
                color: backend.themeMuted
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "v0.1.0"
                font.family: "iA Writer Mono S"
                font.pixelSize: 10
                color: backend.themeMuted
            }
        }
    }

    // Add Topic Modal
    AddTopicModal {
        id: addTopicModal
    }

    // Edit Notes Modal
    EditNotesModal {
        id: editNotesModal
    }

    // Delete Confirmation Dialog
    Dialog {
        id: deleteConfirmDialog
        title: "Delete Topic"
        modal: true
        anchors.centerIn: parent
        width: 380
        padding: 20

        property int topicIdToDelete: -1

        background: Rectangle {
            color: backend.themeLighterBg
            radius: 8
            border.color: backend.darkMode ? "#343a40" : "#d1d5db"
        }

        contentItem: Label {
            text: "Are you sure you want to delete this topic and all its scheduled reviews?"
            font.family: "iA Writer Mono S"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            color: backend.themeForeground
        }

        footer: DialogButtonBox {
            background: Rectangle { color: "transparent" }

            Button {
                id: cancelDelButton
                text: "Cancel"
                flat: true
                font.family: "iA Writer Mono S"
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                contentItem: Text {
                    text: cancelDelButton.text
                    font: cancelDelButton.font
                    color: backend.themeMuted
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: deleteConfirmDialog.reject()
            }

            Button {
                id: deleteButton
                text: "Delete"
                highlighted: true
                font.family: "iA Writer Mono S"
                font.bold: true
                Material.background: "#dc2626"
                Material.foreground: "#ffffff"
                contentItem: Text {
                    text: deleteButton.text
                    font: deleteButton.font
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: {
                    if (deleteConfirmDialog.topicIdToDelete > 0) {
                        backend.deleteTopic(deleteConfirmDialog.topicIdToDelete);
                    }
                    deleteConfirmDialog.accept();
                }
            }
        }
    }
}
