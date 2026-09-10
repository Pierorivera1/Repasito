import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root
    height: 64
    color: backend.themeLighterBg
    radius: 8
    border.color: backend.darkMode ? "#282c30" : "#e5e7eb"
    border.width: 1

    property string selectedDate: ""
    signal dateSelected(string date)

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        Repeater {
            model: backend.dayStrip

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 6
                color: {
                    if (modelData.date === root.selectedDate) {
                        return backend.themeSelection;
                    }
                    if (modelData.isToday) {
                        return backend.darkMode ? "#24292d" : "#e9ecef";
                    }
                    return mouseArea.containsMouse ? (backend.darkMode ? "#1d2125" : "#f1f3f5") : "transparent";
                }
                border.color: modelData.isToday ? backend.themeAccent : "transparent"
                border.width: modelData.isToday ? 1 : 0

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: modelData.dayOfWeek
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        color: modelData.isToday ? backend.themeAccent : backend.themeMuted
                        Layout.alignment: Qt.AlignHCenter
                    }

                    RowLayout {
                        spacing: 4
                        Layout.alignment: Qt.AlignHCenter

                        Text {
                            text: modelData.dayNumber
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 14
                            font.bold: modelData.isToday
                            color: backend.themeForeground
                        }

                        // Badge indicator if there are pending reviews
                        Rectangle {
                            visible: modelData.reviewCount > 0
                            width: 14
                            height: 14
                            radius: 7
                            color: modelData.isToday ? backend.themeAccent : (backend.darkMode ? "#3e444a" : "#ced4da")

                            Text {
                                anchors.centerIn: parent
                                text: modelData.reviewCount > 9 ? "9+" : modelData.reviewCount
                                font.family: "iA Writer Mono S"
                                font.pixelSize: 9
                                font.bold: true
                                color: modelData.isToday ? (backend.darkMode ? "#101315" : "#ffffff") : backend.themeForeground
                            }
                        }
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.selectedDate === modelData.date) {
                            root.selectedDate = "";
                        } else {
                            root.selectedDate = modelData.date;
                        }
                        root.dateSelected(root.selectedDate);
                    }
                }
            }
        }
    }
}
