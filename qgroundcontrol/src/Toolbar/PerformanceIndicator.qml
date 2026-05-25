import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Item {
    id:             control
    width:          perfRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: true

    property var _sysMonitor: QGroundControl.systemResourceMonitor
    property int _fps: 0
    property int _frameCount: 0

    QGCPalette { id: qgcPal }

    FrameAnimation {
        id: frameAnimation
        running: true
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            _fps = frameAnimation.frames
            frameAnimation.frames = 0
        }
    }

    Row {
        id:             perfRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            QGCLabel {
                font.pointSize: ScreenTools.smallFontPointSize
                color:          qgcPal.text
                text:           _fps + " FPS"
            }
            QGCLabel {
                font.pointSize: ScreenTools.smallFontPointSize
                color:          qgcPal.text
                text:           "CPU " + (_sysMonitor ? _sysMonitor.cpuUsage.toFixed(0) : "—") + "%"
            }
            QGCLabel {
                font.pointSize: ScreenTools.smallFontPointSize
                color:          qgcPal.text
                text:           "MEM " + (_sysMonitor ? _sysMonitor.memoryUsagePercent.toFixed(0) : "—") + "%"
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(perfInfoPage, control)
    }

    Component {
        id: perfInfoPage
        PerformanceIndicatorPage {
            fps: _fps
        }
    }
}
