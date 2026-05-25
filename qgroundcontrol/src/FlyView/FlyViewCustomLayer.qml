import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: _root

    property var parentToolInsets
    property var totalToolInsets:   _toolInsets
    property var mapControl

    property bool   _imageWindowVisible:    false
    property string _imagePath:             ""
    property string _preConfiguredPath:     ""
    property real   _windowWidth:           ScreenTools.defaultFontPixelHeight * 20
    property real   _windowHeight:          ScreenTools.defaultFontPixelHeight * 15
    property real   _titleBarHeight:        ScreenTools.defaultFontPixelHeight * 1

    property bool   _scriptPanelVisible:    false
    property real   _scriptPanelWidth:      ScreenTools.defaultFontPixelHeight * 25
    property real   _scriptPanelHeight:     ScreenTools.defaultFontPixelHeight * 18
    property var    _scriptRunner:          QGroundControl.scriptRunner

    Component.onCompleted: {
        if (_preConfiguredPath !== "") {
            _imagePath = _preConfiguredPath
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }

    QGCButton {
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        text:               qsTr("Image")
        visible:            !_imageWindowVisible
        onClicked:          _imageWindowVisible = true
    }

    QGCButton {
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 2.5
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        text:               qsTr("Script")
        visible:            !_scriptPanelVisible
        onClicked:          _scriptPanelVisible = true
    }

    QGCFileDialog {
        id:             imageFileDialog
        title:          qsTr("Select Image")
        nameFilters:    [qsTr("Image files (*.png *.jpg *.jpeg *.bmp *.gif *.svg)"), qsTr("All Files (*)")]
        onAcceptedForLoad: (file) => {
            _imagePath = file
        }
    }

    QGCMovableItem {
        id:             imageWindow
        x:              ScreenTools.defaultFontPixelHeight * 5
        y:              ScreenTools.defaultFontPixelHeight * 2
        width:          _windowWidth
        height:         _windowHeight
        visible:        _imageWindowVisible
        minimumWidth:   ScreenTools.defaultFontPixelHeight * 10
        minimumHeight:  ScreenTools.defaultFontPixelHeight * 8

        onResetRequested: {
            x = ScreenTools.defaultFontPixelHeight * 5
            y = ScreenTools.defaultFontPixelHeight * 2
            tForm.xScale = 1
            tForm.yScale = 1
        }

        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window
            radius:         ScreenTools.defaultFontPixelHeight / 4
            border.color:   qgcPal.groupBorder
            border.width:   1

            Column {
                anchors.fill: parent

                Rectangle {
                    width:  parent.width
                    height: _titleBarHeight
                    color:  qgcPal.windowShade
                    radius: ScreenTools.defaultFontPixelHeight / 4

                    Rectangle {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.bottom: parent.bottom
                        height:         parent.radius
                        color:          parent.color
                    }

                    RowLayout {
                        anchors.fill:       parent
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 0.5

                        QGCLabel {
                            Layout.fillWidth:   true
                            text:               qsTr("Image Overlay")
                            font.pointSize:     ScreenTools.smallFontPointSize
                        }

                        QGCColoredImage {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 0.8
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                            source:                 "/InstrumentValueIcons/folder-outline.svg"
                            color:                  qgcPal.text
                            fillMode:               Image.PreserveAspectFit

                            MouseArea {
                                anchors.fill:   parent
                                onClicked:      imageFileDialog.openForLoad()
                            }
                        }

                        QGCColoredImage {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 0.8
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                            source:                 "/InstrumentValueIcons/close.svg"
                            color:                  qgcPal.text
                            fillMode:               Image.PreserveAspectFit

                            MouseArea {
                                anchors.fill:   parent
                                onClicked:      _imageWindowVisible = false
                            }
                        }
                    }
                }

                Item {
                    width:  parent.width
                    height: parent.height - _titleBarHeight

                    Image {
                        anchors.fill:       parent
                        anchors.margins:    ScreenTools.defaultFontPixelWidth / 2
                        source:             _imagePath !== "" ? "file:///" + _imagePath : ""
                        fillMode:           Image.PreserveAspectFit
                        visible:            _imagePath !== ""
                        asynchronous:       true
                    }

                    QGCLabel {
                        anchors.centerIn:       parent
                        text:                   qsTr("No image loaded\nClick folder icon to select")
                        horizontalAlignment:    Text.AlignHCenter
                        visible:                _imagePath === ""
                    }
                }
            }
        }
    }

    QGCFileDialog {
        id:             scriptFileDialog
        title:          qsTr("Select Script")
        nameFilters:    [qsTr("Script files (*.bat *.cmd *.py *.ps1 *.sh)"), qsTr("All Files (*)")]
        onAcceptedForLoad: (file) => {
            _scriptRunner.runScript(file)
        }
    }

    QGCMovableItem {
        id:             scriptPanel
        x:              ScreenTools.defaultFontPixelHeight * 8
        y:              ScreenTools.defaultFontPixelHeight * 3
        width:          _scriptPanelWidth
        height:         _scriptPanelHeight
        visible:        _scriptPanelVisible
        minimumWidth:   ScreenTools.defaultFontPixelHeight * 15
        minimumHeight:  ScreenTools.defaultFontPixelHeight * 10

        onResetRequested: {
            x = ScreenTools.defaultFontPixelHeight * 8
            y = ScreenTools.defaultFontPixelHeight * 3
            tForm.xScale = 1
            tForm.yScale = 1
        }

        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window
            radius:         ScreenTools.defaultFontPixelHeight / 4
            border.color:   qgcPal.groupBorder
            border.width:   1

            Column {
                anchors.fill: parent

                Rectangle {
                    width:  parent.width
                    height: ScreenTools.defaultFontPixelHeight * 1.5
                    color:  qgcPal.windowShade
                    radius: ScreenTools.defaultFontPixelHeight / 4

                    Rectangle {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.bottom: parent.bottom
                        height:         parent.radius
                        color:          parent.color
                    }

                    RowLayout {
                        anchors.fill:       parent
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 0.5

                        QGCLabel {
                            Layout.fillWidth:   true
                            text:               qsTr("Script Runner")
                            font.pointSize:     ScreenTools.smallFontPointSize
                        }

                        QGCColoredImage {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight
                            source:                 "/InstrumentValueIcons/folder-outline.svg"
                            color:                  qgcPal.text
                            fillMode:               Image.PreserveAspectFit
                            MouseArea {
                                anchors.fill: parent
                                onClicked:    scriptFileDialog.openForLoad()
                            }
                        }

                        QGCColoredImage {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight
                            source:                 "/InstrumentValueIcons/close.svg"
                            color:                  qgcPal.text
                            fillMode:               Image.PreserveAspectFit
                            MouseArea {
                                anchors.fill: parent
                                onClicked:    _scriptPanelVisible = false
                            }
                        }
                    }
                }

                Item {
                    width:  parent.width
                    height: parent.height - ScreenTools.defaultFontPixelHeight * 1.5 - scriptButtons.height

                    QGCFlickable {
                        anchors.fill:       parent
                        anchors.margins:    ScreenTools.defaultFontPixelWidth / 2
                        contentHeight:      outputText.height
                        clip:               true

                        QGCLabel {
                            id:             outputText
                            width:          parent.width
                            text:           _scriptRunner ? _scriptRunner.output : ""
                            wrapMode:       Text.WrapAnywhere
                            font.pointSize: ScreenTools.smallFontPointSize
                            font.family:    "Courier New"
                        }
                    }
                }

                Row {
                    id:         scriptButtons
                    width:      parent.width
                    height:     ScreenTools.defaultFontPixelHeight * 1.8
                    spacing:    ScreenTools.defaultFontPixelWidth / 2
                    leftPadding: ScreenTools.defaultFontPixelWidth / 2

                    QGCButton {
                        text:       qsTr("Stop")
                        enabled:    _scriptRunner && _scriptRunner.running
                        onClicked:  _scriptRunner.stopScript()
                        height:     parent.height * 0.9
                    }

                    QGCButton {
                        text:       qsTr("Clear")
                        onClicked:  _scriptRunner.clearOutput()
                        height:     parent.height * 0.9
                    }
                }
            }
        }
    }
}