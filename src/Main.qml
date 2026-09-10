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

    readonly property bool isAnyModalOpen: addTopicModal.visible || editNotesModal.visible || deleteConfirmDialog.visible

    // Keyboard Shortcuts
    Shortcut {
        sequence: "Ctrl+N"
        enabled: !isAnyModalOpen
        onActivated: addTopicModal.openModal()
    }
    Shortcut {
        sequence: "N"
        enabled: !searchField.activeFocus && !isAnyModalOpen
        onActivated: addTopicModal.openModal()
    }
    Shortcut {
        sequence: "/"
        enabled: !searchField.activeFocus && !isAnyModalOpen
        onActivated: {
            searchField.forceActiveFocus();
            searchField.selectAll();
        }
    }
    Shortcut {
        sequence: "Escape"
        enabled: !isAnyModalOpen
        onActivated: {
            if (searchField.activeFocus || searchField.text.length > 0) {
                searchField.text = "";
                agendaView.forceActiveFocus();
            } else if (backend.selectedDate.length > 0) {
                backend.clearSelectedDate();
            }
        }
    }
    Shortcut {
        sequence: "H"
        enabled: !searchField.activeFocus && !isAnyModalOpen
        onActivated: backend.previousDay()
    }
    Shortcut {
        sequence: "L"
        enabled: !searchField.activeFocus && !isAnyModalOpen
        onActivated: backend.nextDay()
    }
    Shortcut {
        sequence: "J"
        enabled: !searchField.activeFocus && !isAnyModalOpen
        onActivated: agendaView.incrementCurrentIndex()
    }
    Shortcut {
        sequence: "K"
        enabled: !searchField.activeFocus && !isAnyModalOpen
        onActivated: agendaView.decrementCurrentIndex()
    }
    Shortcut {
        sequence: "Space"
        enabled: !searchField.activeFocus && !isAnyModalOpen && agendaView.currentItem !== null
        onActivated: {
            var item = agendaView.model[agendaView.currentIndex];
            if (item) {
                backend.toggleReview(item.reviewId, !item.isCompleted);
            }
        }
    }
    Shortcut {
        sequence: "D"
        enabled: !searchField.activeFocus && !isAnyModalOpen && agendaView.currentItem !== null
        onActivated: {
            var item = agendaView.model[agendaView.currentIndex];
            if (item) {
                deleteConfirmDialog.topicIdToDelete = item.topicId;
                deleteConfirmDialog.topicTitleToDelete = item.title || "";
                deleteConfirmDialog.open();
            }
        }
    }
    Shortcut {
        sequence: "Delete"
        enabled: !searchField.activeFocus && !isAnyModalOpen && agendaView.currentItem !== null
        onActivated: {
            var item = agendaView.model[agendaView.currentIndex];
            if (item) {
                deleteConfirmDialog.topicIdToDelete = item.topicId;
                deleteConfirmDialog.topicTitleToDelete = item.title || "";
                deleteConfirmDialog.open();
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
                objectName: "searchField"
                placeholderText: "Search (/)"
                font.family: "iA Writer Mono S"
                font.pixelSize: 12
                color: backend.themeForeground
                Material.accent: backend.themeAccent
                implicitWidth: 170
                rightPadding: clearSearchBtn.visible ? 24 : 8
                selectByMouse: true
                background: Rectangle {
                    color: backend.themeLighterBg
                    radius: 6
                    border.color: searchField.activeFocus ? backend.themeAccent : (backend.darkMode ? "#282c30" : "#e5e7eb")
                    border.width: 1
                }
                onTextChanged: backend.searchQuery = text
                onAccepted: agendaView.forceActiveFocus()

                Connections {
                    target: backend
                    function onSearchQueryChanged() {
                        if (searchField.text !== backend.searchQuery) {
                            searchField.text = backend.searchQuery;
                        }
                    }
                }

                Text {
                    id: clearSearchBtn
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    visible: searchField.text.length > 0
                    text: "✕"
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 10
                    font.bold: true
                    color: clearSearchMouse.containsMouse ? backend.themeForeground : backend.themeMuted

                    MouseArea {
                        id: clearSearchMouse
                        anchors.fill: parent
                        anchors.margins: -4
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        ToolTip.visible: containsMouse
                        ToolTip.text: "Clear search"
                        onClicked: {
                            searchField.text = "";
                        }
                    }
                }
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
            objectName: "dayStrip"
            Layout.fillWidth: true
        }

        // Active Date Filter Indicator / Clear Badge
        RowLayout {
            Layout.fillWidth: true
            visible: backend.selectedDate.length > 0
            spacing: 8

            Rectangle {
                height: 28
                implicitHeight: 28
                radius: 6
                color: backend.themeLighterBg
                border.color: backend.themeAccent
                border.width: 1
                implicitWidth: filterBadgeContent.implicitWidth + 20

                RowLayout {
                    id: filterBadgeContent
                    anchors.centerIn: parent
                    spacing: 8

                    Text {
                        text: "Filtered by: " + backend.selectedDateDisplay
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 11
                        font.bold: true
                        color: backend.themeForeground
                    }

                    Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        width: 16
                        height: 16
                        radius: 8
                        color: closeBtnMouse.containsMouse ? backend.themeAccent : (backend.darkMode ? "#282c30" : "#e5e7eb")

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 9
                            font.bold: true
                            color: closeBtnMouse.containsMouse ? backend.themeAccentForeground : backend.themeMuted
                        }

                        MouseArea {
                            id: closeBtnMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            ToolTip.visible: containsMouse
                            ToolTip.text: "Clear date filter"
                            onClicked: backend.clearSelectedDate()
                        }
                    }
                }
            }

            Text {
                text: (backend.selectedDate === backend.todayDateIso) ? "Showing reviews for today and overdue" : "Showing only reviews for this date"
                font.family: "iA Writer Mono S"
                font.pixelSize: 11
                color: backend.themeMuted
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Button {
                id: clearFilterBtn
                text: "Clear Filter"
                flat: true
                font.family: "iA Writer Mono S"
                font.pixelSize: 11
                implicitHeight: 28
                Layout.preferredHeight: 28
                Layout.alignment: Qt.AlignVCenter
                contentItem: Text {
                    text: clearFilterBtn.text
                    font: clearFilterBtn.font
                    color: clearFilterBtn.hovered ? backend.themeAccent : backend.themeMuted
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: backend.clearSelectedDate()
            }
        }

        // Agenda List
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"

            ListView {
                id: agendaView
                objectName: "agendaView"
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
                    onDeleteRequested: function(topicId, title) {
                        deleteConfirmDialog.topicIdToDelete = topicId;
                        deleteConfirmDialog.topicTitleToDelete = title || "";
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
                        text: {
                            if (searchField.text.trim().length > 0) return "🔍";
                            if (backend.selectedDate.length > 0) return "📅";
                            return "🎉";
                        }
                        font.pixelSize: 36
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: {
                            if (searchField.text.trim().length > 0 && backend.selectedDate.length > 0) {
                                return "No reviews matching search on this day";
                            }
                            if (searchField.text.trim().length > 0) {
                                return "No reviews matching search";
                            }
                            if (backend.selectedDate.length > 0) {
                                return "No reviews scheduled for this day";
                            }
                            return "No pending reviews";
                        }
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        color: backend.themeForeground
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: {
                            if (searchField.text.trim().length > 0 && backend.selectedDate.length > 0) {
                                return "Try adjusting your search query or clearing filters to see other reviews.";
                            }
                            if (searchField.text.trim().length > 0) {
                                return "Try adjusting your search query or clear the search.";
                            }
                            if (backend.selectedDate.length > 0) {
                                return "Select another day or click the active day to restore the full agenda.";
                            }
                            return "Press 'N' or click '+ Add Topic' to schedule reviews.";
                        }
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 12
                        color: backend.themeMuted
                        Layout.alignment: Qt.AlignHCenter
                    }

                    RowLayout {
                        visible: backend.selectedDate.length > 0 || searchField.text.trim().length > 0
                        spacing: 8
                        Layout.alignment: Qt.AlignHCenter

                        Button {
                            id: emptyClearSearchBtn
                            visible: searchField.text.trim().length > 0
                            text: "Clear Search"
                            flat: true
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 12
                            contentItem: Text {
                                text: emptyClearSearchBtn.text
                                font: emptyClearSearchBtn.font
                                color: backend.themeAccent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                searchField.text = "";
                            }
                        }

                        Button {
                            id: emptyClearDateBtn
                            visible: backend.selectedDate.length > 0
                            text: "Clear Date Filter"
                            flat: true
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 12
                            contentItem: Text {
                                text: emptyClearDateBtn.text
                                font: emptyClearDateBtn.font
                                color: backend.themeAccent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                backend.clearSelectedDate();
                            }
                        }

                        Button {
                            id: emptyClearAllBtn
                            visible: backend.selectedDate.length > 0 && searchField.text.trim().length > 0
                            text: "Clear All Filters"
                            flat: true
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 12
                            contentItem: Text {
                                text: emptyClearAllBtn.text
                                font: emptyClearAllBtn.font
                                color: backend.themeMuted
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                searchField.text = "";
                                backend.clearSelectedDate();
                            }
                        }
                    }
                }
            }
        }

        // Bottom status / tip bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Shortcuts: [N] Add topic   [/] Search   [H/L] Day   [J/K] Navigate   [D] Delete   [Space] Complete"
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
        objectName: "addTopicModal"
    }

    // Edit Notes Modal
    EditNotesModal {
        id: editNotesModal
        objectName: "editNotesModal"
    }

    // Delete Confirmation Dialog
    Dialog {
        id: deleteConfirmDialog
        objectName: "deleteConfirmDialog"
        title: "Delete Topic"
        modal: true
        anchors.centerIn: parent
        width: 400
        padding: 20

        property int topicIdToDelete: -1
        property string topicTitleToDelete: ""

        onAccepted: {
            if (topicIdToDelete > 0) {
                backend.deleteTopic(topicIdToDelete);
                topicIdToDelete = -1;
            }
        }

        onRejected: {
            topicIdToDelete = -1;
        }

        background: Rectangle {
            color: backend.themeLighterBg
            radius: 8
            border.color: backend.darkMode ? "#343a40" : "#d1d5db"
        }

        contentItem: Label {
            text: deleteConfirmDialog.topicTitleToDelete.length > 0
                ? ("Are you sure you want to delete \"" + deleteConfirmDialog.topicTitleToDelete + "\" and all its scheduled reviews across all dates?")
                : "Are you sure you want to delete this topic and all its scheduled reviews across all dates?"
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
                onClicked: deleteConfirmDialog.accept()
            }
        }
    }
}
