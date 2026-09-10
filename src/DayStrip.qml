import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: root
    height: 64
    implicitHeight: 64
    color: backend.themeLighterBg
    radius: 8
    border.color: backend.darkMode ? "#282c30" : "#e5e7eb"
    border.width: 1

    property string selectedDate: backend.selectedDate
    signal dateSelected(string date)

    onSelectedDateChanged: {
        if (backend.selectedDate !== root.selectedDate) {
            backend.selectedDate = root.selectedDate;
        }
    }

    Connections {
        target: backend
        function onSelectedDateChanged() {
            if (root.selectedDate !== backend.selectedDate) {
                root.selectedDate = backend.selectedDate;
            }
        }
    }

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
                    if (modelData.date === backend.selectedDate) {
                        return backend.themeSelection;
                    }
                    if (modelData.isToday) {
                        return backend.darkMode ? "#24292d" : "#e9ecef";
                    }
                    return mouseArea.containsMouse ? (backend.darkMode ? "#1d2125" : "#f1f3f5") : "transparent";
                }
                border.color: {
                    if (modelData.date === backend.selectedDate) {
                        return backend.themeAccent;
                    }
                    if (modelData.isToday) {
                        return backend.darkMode ? "#3a4146" : "#ced4da";
                    }
                    return "transparent";
                }
                border.width: (modelData.date === backend.selectedDate) ? 2 : (modelData.isToday ? 1 : 0)

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: modelData.dayOfWeek
                        font.family: "iA Writer Mono S"
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        color: (modelData.date === backend.selectedDate || modelData.isToday) ? backend.themeAccent : backend.themeMuted
                        Layout.alignment: Qt.AlignHCenter
                    }

                    RowLayout {
                        spacing: 4
                        Layout.alignment: Qt.AlignHCenter

                        Text {
                            text: modelData.dayNumber
                            font.family: "iA Writer Mono S"
                            font.pixelSize: 14
                            font.bold: (modelData.isToday || modelData.date === backend.selectedDate)
                            color: backend.themeForeground
                        }

                        // Badge indicator if there are pending reviews
                        Rectangle {
                            visible: modelData.reviewCount > 0
                            height: 14
                            implicitHeight: 14
                            width: Math.max(14, badgeCountText.implicitWidth + 6)
                            implicitWidth: width
                            radius: 7
                            color: modelData.isToday ? backend.themeAccent : (backend.darkMode ? "#3e444a" : "#ced4da")

                            Text {
                                id: badgeCountText
                                anchors.centerIn: parent
                                text: modelData.reviewCount > 9 ? "9+" : modelData.reviewCount
                                font.family: "iA Writer Mono S"
                                font.pixelSize: 9
                                font.bold: true
                                color: modelData.isToday ? backend.themeAccentForeground : backend.themeForeground
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
                        var newDate = (backend.selectedDate === modelData.date) ? "" : modelData.date;
                        backend.selectedDate = newDate;
                        root.dateSelected(newDate);
                    }
                }
            }
        }
    }
}
