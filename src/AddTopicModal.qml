import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: root
    title: "New Review Topic"
    modal: true
    anchors.centerIn: parent
    width: Math.min(520, parent.width - 40)
    padding: 20

    background: Rectangle {
        color: backend.themeLighterBg
        radius: 10
        border.color: backend.darkMode ? "#343a40" : "#d1d5db"
        border.width: 1
    }

    header: Label {
        text: "Schedule Spaced Repetition"
        font.family: "iA Writer Mono S"
        font.pixelSize: 16
        font.bold: true
        color: backend.themeForeground
        padding: 16
        bottomPadding: 0
    }

    property string initialDateIso: backend.todayDateIso

    function openModal() {
        titleField.text = "";
        notesField.text = "";
        initialDateIso = backend.todayDateIso;
        dateField.text = backend.todayDateIso;
        root.open();
        titleField.forceActiveFocus();
    }

    function computeReviewDates(baseIso) {
        var d = new Date(baseIso + "T00:00:00");
        if (isNaN(d.getTime())) return ["+1d", "+3d", "+5d"];
        
        function addDays(dt, n) {
            var res = new Date(dt.getTime());
            res.setDate(res.getDate() + n);
            return res.toLocaleDateString(Qt.locale(), "MMM d");
        }

        return [addDays(d, 1), addDays(d, 3), addDays(d, 5)];
    }

    function submit() {
        if (titleField.text.trim().length > 0) {
            backend.addTopic(titleField.text.trim(), notesField.text, root.initialDateIso);
            root.accept();
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 14

        // Topic Title
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: "TOPIC OR MODULE"
                font.family: "iA Writer Mono S"
                font.pixelSize: 11
                color: backend.themeMuted
            }

            TextField {
                id: titleField
                objectName: "titleField"
                Layout.fillWidth: true
                font.family: "iA Writer Mono S"
                font.pixelSize: 14
                color: backend.themeForeground
                Material.accent: backend.themeAccent
                selectByMouse: true
                onAccepted: notesField.forceActiveFocus()
                KeyNavigation.tab: notesField
                Keys.onTabPressed: function(event) {
                    notesField.forceActiveFocus();
                    event.accepted = true;
                }
            }
        }

        // Initial Study Date
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: "STUDIED ON"
                font.family: "iA Writer Mono S"
                font.pixelSize: 11
                color: backend.themeMuted
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    id: dateField
                    objectName: "dateField"
                    text: root.initialDateIso
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 13
                    color: backend.themeForeground
                    Material.accent: backend.themeAccent
                    implicitWidth: 130
                    activeFocusOnTab: false
                    KeyNavigation.tab: notesField
                    KeyNavigation.backtab: titleField
                    onTextChanged: {
                        root.initialDateIso = text.trim();
                    }
                }

                Button {
                    text: "Today"
                    flat: true
                    focusPolicy: Qt.ClickFocus
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 11
                    onClicked: {
                        root.initialDateIso = backend.todayDateIso;
                        dateField.text = backend.todayDateIso;
                    }
                }

                Button {
                    text: "Yesterday"
                    flat: true
                    focusPolicy: Qt.ClickFocus
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 11
                    onClicked: {
                        var d = new Date();
                        d.setDate(d.getDate() - 1);
                        var yyyy = d.getFullYear();
                        var mm = String(d.getMonth() + 1).padStart(2, '0');
                        var dd = String(d.getDate()).padStart(2, '0');
                        var iso = yyyy + "-" + mm + "-" + dd;
                        root.initialDateIso = iso;
                        dateField.text = iso;
                    }
                }
            }
        }

        // Preview of 3 Review Stages
        Rectangle {
            Layout.fillWidth: true
            height: 48
            radius: 6
            color: backend.darkMode ? "#141719" : "#f1f5f9"
            border.color: backend.darkMode ? "#24282c" : "#e2e8f0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                property var dates: root.computeReviewDates(root.initialDateIso)

                Label {
                    text: "Repetition plan:"
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 11
                    color: backend.themeMuted
                }

                Rectangle {
                    radius: 4
                    color: backend.themeSelection
                    height: 24
                    width: r1Label.implicitWidth + 12
                    Label {
                        id: r1Label
                        anchors.centerIn: parent
                        text: "R1: " + parent.parent.dates[0] + " (+1d)"
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 11
                        font.bold: true
                        color: backend.themeForeground
                    }
                }

                Rectangle {
                    radius: 4
                    color: backend.themeSelection
                    height: 24
                    width: r2Label.implicitWidth + 12
                    Label {
                        id: r2Label
                        anchors.centerIn: parent
                        text: "R2: " + parent.parent.dates[1] + " (+3d)"
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 11
                        font.bold: true
                        color: backend.themeForeground
                    }
                }

                Rectangle {
                    radius: 4
                    color: backend.themeSelection
                    height: 24
                    width: r3Label.implicitWidth + 12
                    Label {
                        id: r3Label
                        anchors.centerIn: parent
                        text: "R3: " + parent.parent.dates[2] + " (+5d)"
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 11
                        font.bold: true
                        color: backend.themeForeground
                    }
                }
            }
        }

        // Markdown Notes Area
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "NOTES (MARKDOWN)"
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 11
                    color: backend.themeMuted
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: "↵ Schedule  ⇧↵ Newline"
                    font.family: "iA Writer Mono S"
                    font.pixelSize: 10
                    color: backend.themeMuted
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                color: backend.darkMode ? "#121517" : "#f8f9fa"
                border.color: backend.darkMode ? "#282c30" : "#d1d5db"
                border.width: 1
                radius: 4

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true

                    TextArea {
                        id: notesField
                        objectName: "notesField"
                        placeholderText: "- [ ] Checklist items\n- Concepts to recall\n- https://reference.link"
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 12
                        color: backend.themeForeground
                        placeholderTextColor: backend.themeMuted
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        topPadding: 6
                        bottomPadding: 6
                        leftPadding: 6
                        rightPadding: 6
                        background: null

                        Keys.onReturnPressed: function(event) {
                            if (event.modifiers & Qt.ShiftModifier) {
                                event.accepted = false;
                            } else {
                                event.accepted = true;
                                root.submit();
                            }
                        }
                        Keys.onEnterPressed: function(event) {
                            if (event.modifiers & Qt.ShiftModifier) {
                                event.accepted = false;
                            } else {
                                event.accepted = true;
                                root.submit();
                            }
                        }
                        KeyNavigation.backtab: titleField
                        Keys.onBacktabPressed: function(event) {
                            titleField.forceActiveFocus();
                            event.accepted = true;
                        }
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        background: Rectangle {
            color: "transparent"
        }

        Button {
            id: cancelButton
            text: "Cancel"
            flat: true
            font.family: "iA Writer Mono S"
            font.pixelSize: 12
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            contentItem: Text {
                text: cancelButton.text
                font: cancelButton.font
                color: backend.themeMuted
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: root.reject()
        }

        Button {
            id: scheduleButton
            objectName: "scheduleButton"
            text: "Schedule Reviews"
            highlighted: true
            font.family: "iA Writer Mono S"
            font.pixelSize: 12
            font.bold: true
            Material.background: backend.themeAccent
            Material.foreground: backend.themeAccentForeground
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

            contentItem: Text {
                text: scheduleButton.text
                font: scheduleButton.font
                color: backend.themeAccentForeground
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: root.submit()
        }
    }
}
