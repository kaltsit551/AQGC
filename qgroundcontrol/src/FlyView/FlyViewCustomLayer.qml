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

    property bool   _aiPanelVisible:        false
    property bool   _aiConfigVisible:       false
    property real   _aiPanelWidth:          ScreenTools.defaultFontPixelHeight * 25
    property real   _aiPanelHeight:         ScreenTools.defaultFontPixelHeight * 22
    property var    _aiChat:                QGroundControl.aiChatService

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

    QGCButton {
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 5
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        text:               qsTr("AI")
        visible:            !_aiPanelVisible
        onClicked:          _aiPanelVisible = true
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
                    id:     imageContainer
                    width:  parent.width
                    height: parent.height - _titleBarHeight
                    clip:   true

                    property real imageScale: 1.0
                    property real minScale:   0.5
                    property real maxScale:   10.0

                    Flickable {
                        id:             imageFlickable
                        anchors.fill:   parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth / 2
                        contentWidth:   overlayImage.width
                        contentHeight:  overlayImage.height
                        clip:           true
                        visible:        _imagePath !== ""
                        boundsBehavior: Flickable.StopAtBounds

                        Image {
                            id:             overlayImage
                            source:         _imagePath !== "" ? "file:///" + _imagePath : ""
                            fillMode:       Image.PreserveAspectFit
                            asynchronous:   true
                            width:          imageFlickable.width * imageContainer.imageScale
                            height:         imageFlickable.height * imageContainer.imageScale
                        }

                        MouseArea {
                            anchors.fill:   parent
                            onWheel: (wheel) => {
                                var oldScale = imageContainer.imageScale
                                var factor = wheel.angleDelta.y > 0 ? 1.15 : (1 / 1.15)
                                var newScale = Math.max(imageContainer.minScale,
                                    Math.min(imageContainer.maxScale, oldScale * factor))
                                if (newScale === oldScale) return

                                var flickX = wheel.x - imageFlickable.contentX
                                var flickY = wheel.y - imageFlickable.contentY
                                var ratio = newScale / oldScale

                                imageContainer.imageScale = newScale

                                imageFlickable.contentX = Math.max(0,
                                    Math.min(imageFlickable.contentWidth - imageFlickable.width,
                                        wheel.x * ratio - flickX))
                                imageFlickable.contentY = Math.max(0,
                                    Math.min(imageFlickable.contentHeight - imageFlickable.height,
                                        wheel.y * ratio - flickY))
                            }
                            onPressed: (mouse) => mouse.accepted = false
                        }
                    }

                    Row {
                        anchors.bottom:         parent.bottom
                        anchors.right:          parent.right
                        anchors.margins:        ScreenTools.defaultFontPixelWidth / 2
                        spacing:                ScreenTools.defaultFontPixelWidth / 2
                        visible:                _imagePath !== ""

                        QGCButton {
                            text:       "-"
                            width:      ScreenTools.defaultFontPixelHeight * 1.5
                            height:     width
                            onClicked: {
                                imageContainer.imageScale = Math.max(imageContainer.minScale,
                                    imageContainer.imageScale / 1.3)
                            }
                        }

                        QGCButton {
                            text:       Math.round(imageContainer.imageScale * 100) + "%"
                            height:     ScreenTools.defaultFontPixelHeight * 1.5
                            onClicked:  imageContainer.imageScale = 1.0
                        }

                        QGCButton {
                            text:       "+"
                            width:      ScreenTools.defaultFontPixelHeight * 1.5
                            height:     width
                            onClicked: {
                                imageContainer.imageScale = Math.min(imageContainer.maxScale,
                                    imageContainer.imageScale * 1.3)
                            }
                        }
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

    // __AI_PANEL_PLACEHOLDER__
    QGCMovableItem {
        id:             aiPanel
        x:              ScreenTools.defaultFontPixelHeight * 11
        y:              ScreenTools.defaultFontPixelHeight * 4
        width:          _aiPanelWidth
        height:         _aiPanelHeight
        visible:        _aiPanelVisible
        minimumWidth:   ScreenTools.defaultFontPixelHeight * 16
        minimumHeight:  ScreenTools.defaultFontPixelHeight * 12

        onResetRequested: {
            x = ScreenTools.defaultFontPixelHeight * 11
            y = ScreenTools.defaultFontPixelHeight * 4
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
                anchors.fill:       parent
                anchors.margins:    1
                spacing:            0
                // __AI_BODY_PLACEHOLDER__
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
                            text:               qsTr("AI Chat")
                            font.pointSize:     ScreenTools.smallFontPointSize
                        }

                        QGCColoredImage {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight
                            source:                 "/InstrumentValueIcons/cog.svg"
                            color:                  _aiConfigVisible ? qgcPal.colorGreen : qgcPal.text
                            fillMode:               Image.PreserveAspectFit
                            MouseArea {
                                anchors.fill: parent
                                onClicked:    _aiConfigVisible = !_aiConfigVisible
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
                                onClicked:    _aiPanelVisible = false
                            }
                        }
                    }
                }
                // __AI_CONFIG_PLACEHOLDER__
                Rectangle {
                    width:      parent.width
                    height:     _aiConfigVisible ? aiConfigColumn.height + ScreenTools.defaultFontPixelWidth : 0
                    visible:    _aiConfigVisible
                    color:      qgcPal.windowShadeDark
                    clip:       true

                    Column {
                        id:                 aiConfigColumn
                        width:              parent.width
                        anchors.top:        parent.top
                        anchors.topMargin:  ScreenTools.defaultFontPixelWidth / 2
                        spacing:            ScreenTools.defaultFontPixelWidth / 2
                        leftPadding:        ScreenTools.defaultFontPixelWidth / 2
                        rightPadding:       ScreenTools.defaultFontPixelWidth / 2

                        QGCLabel {
                            text:           qsTr("Base URL")
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                        QGCTextField {
                            width:          parent.width - ScreenTools.defaultFontPixelWidth
                            text:           _aiChat ? _aiChat.baseUrl : ""
                            onEditingFinished: if (_aiChat) _aiChat.baseUrl = text
                        }
                        QGCLabel {
                            text:           qsTr("API Key")
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                        QGCTextField {
                            width:          parent.width - ScreenTools.defaultFontPixelWidth
                            echoMode:       TextInput.Password
                            text:           _aiChat ? _aiChat.apiKey : ""
                            onEditingFinished: if (_aiChat) _aiChat.apiKey = text
                        }
                        QGCLabel {
                            text:           qsTr("Model")
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                        QGCTextField {
                            width:          parent.width - ScreenTools.defaultFontPixelWidth
                            text:           _aiChat ? _aiChat.model : ""
                            onEditingFinished: if (_aiChat) _aiChat.model = text
                        }
                    }
                }
                // __AI_CHAT_PLACEHOLDER__
                Item {
                    width:  parent.width
                    height: parent.height
                            - ScreenTools.defaultFontPixelHeight * 1.5
                            - (_aiConfigVisible ? aiConfigColumn.height + ScreenTools.defaultFontPixelWidth : 0)
                            - aiInputRow.height

                    QGCFlickable {
                        id:             aiFlickable
                        anchors.fill:       parent
                        anchors.margins:    ScreenTools.defaultFontPixelWidth / 2
                        contentHeight:      aiConversation.height
                        clip:               true

                        QGCLabel {
                            id:             aiConversation
                            width:          parent.width
                            text:           _aiChat ? _aiChat.conversation : ""
                            wrapMode:       Text.Wrap
                            font.pointSize: ScreenTools.smallFontPointSize
                            onTextChanged:  aiFlickable.contentY = Math.max(0, aiConversation.height - aiFlickable.height)
                        }
                    }
                }

                Row {
                    id:         aiInputRow
                    width:      parent.width
                    height:     ScreenTools.defaultFontPixelHeight * 1.8
                    spacing:    ScreenTools.defaultFontPixelWidth / 2
                    leftPadding: ScreenTools.defaultFontPixelWidth / 2
                    rightPadding: ScreenTools.defaultFontPixelWidth / 2

                    function sendCurrent() {
                        if (_aiChat && !_aiChat.busy && aiInput.text.trim() !== "") {
                            _aiChat.sendMessage(aiInput.text)
                            aiInput.clear()
                        }
                    }

                    QGCTextField {
                        id:             aiInput
                        width:          parent.width - sendButton.width - stopButton.width - clearButton.width
                                        - ScreenTools.defaultFontPixelWidth * 2.5
                        height:         parent.height * 0.9
                        enabled:        _aiChat && !_aiChat.busy
                        placeholderText: qsTr("Type a message...")
                        onAccepted:     aiInputRow.sendCurrent()
                    }

                    QGCButton {
                        id:         sendButton
                        text:       qsTr("Send")
                        height:     parent.height * 0.9
                        enabled:    _aiChat && !_aiChat.busy
                        onClicked:  aiInputRow.sendCurrent()
                    }

                    QGCButton {
                        id:         stopButton
                        text:       qsTr("Stop")
                        height:     parent.height * 0.9
                        enabled:    _aiChat && _aiChat.busy
                        onClicked:  if (_aiChat) _aiChat.stop()
                    }

                    QGCButton {
                        id:         clearButton
                        text:       qsTr("Clear")
                        height:     parent.height * 0.9
                        onClicked:  if (_aiChat) _aiChat.clearConversation()
                    }
                }
            }
        }
    }
}