import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: root
    title: "Edit Topic Notes"
    modal: true
    anchors.centerIn: parent
    width: Math.min(500, parent.width - 40)
    padding: 20

    property int topicId: -1

    background: Rectangle {
        color: backend.themeLighterBg
        radius: 10
        border.color: backend.darkMode ? "#343a40" : "#d1d5db"
        border.width: 1
    }

    header: Label {
        text: "Topic Notes"
        font.family: "iA Writer Mono S"
        font.pixelSize: 16
        font.bold: true
        color: backend.themeForeground
        padding: 16
        bottomPadding: 0
    }

    function openForTopic(id, initialNotes) {
        topicId = id;
        notesEditor.text = initialNotes;
        root.open();
        notesEditor.forceActiveFocus();
    }

    ColumnLayout {
        width: parent.width
        spacing: 10

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 180

            TextArea {
                id: notesEditor
                font.family: "iA Writer Mono S"
                font.pixelSize: 12
                color: backend.themeForeground
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                background: Rectangle {
                    color: backend.darkMode ? "#121517" : "#f8f9fa"
                    border.color: backend.darkMode ? "#282c30" : "#d1d5db"
                    radius: 4
                }
            }
        }
    }

    footer: DialogButtonBox {
        background: Rectangle {
            color: "transparent"
        }

        Button {
            id: cancelEditButton
            text: "Cancel"
            flat: true
            font.family: "iA Writer Mono S"
            font.pixelSize: 12
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            contentItem: Text {
                text: cancelEditButton.text
                font: cancelEditButton.font
                color: backend.themeMuted
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: root.reject()
        }

        Button {
            id: saveButton
            text: "Save Notes"
            highlighted: true
            font.family: "iA Writer Mono S"
            font.pixelSize: 12
            font.bold: true
            Material.background: backend.themeAccent
            Material.foreground: backend.themeAccentForeground
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

            contentItem: Text {
                text: saveButton.text
                font: saveButton.font
                color: backend.themeAccentForeground
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                if (root.topicId > 0) {
                    backend.updateNotes(root.topicId, notesEditor.text);
                }
                root.accept();
            }
        }
    }
}
