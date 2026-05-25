import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

ToolIndicatorPage {
    showExpand: false

    property int fps: 0
    property var _sysMonitor: QGroundControl.systemResourceMonitor

    contentComponent: SettingsGroupLayout {
        heading: qsTr("Performance")

        LabelledLabel {
            label:      qsTr("FPS:")
            labelText:  fps
        }

        LabelledLabel {
            label:      qsTr("CPU Usage:")
            labelText:  _sysMonitor ? _sysMonitor.cpuUsage.toFixed(1) + "%" : "—"
        }

        LabelledLabel {
            label:      qsTr("Memory Used:")
            labelText:  _sysMonitor ? _sysMonitor.memoryUsageMB.toFixed(0) + " MB" : "—"
        }

        LabelledLabel {
            label:      qsTr("Memory Usage:")
            labelText:  _sysMonitor ? _sysMonitor.memoryUsagePercent.toFixed(1) + "%" : "—"
        }

        LabelledLabel {
            label:      qsTr("Total Memory:")
            labelText:  _sysMonitor ? (_sysMonitor.totalMemoryMB / 1024.0).toFixed(1) + " GB" : "—"
        }
    }
}
