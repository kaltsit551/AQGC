import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

// Self-contained AI chat assistant UI: trigger button + movable chat panel.
// Extracted from FlyViewCustomLayer for maintainability.
Item {
    id: _root

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var    _aiChat:                QGroundControl.aiChatService

    property bool   _aiPanelVisible:        false
    property bool   _aiConfigVisible:       false
    property real   _aiPanelWidth:          ScreenTools.defaultFontPixelHeight * 28
    property real   _aiPanelHeight:         ScreenTools.defaultFontPixelHeight * 26

    // AI chat colour scheme (classic blue). Independent of qgcPal so bubbles
    // look modern in both light and dark themes.
    property bool   _aiDark:        qgcPal.window.hslLightness < 0.5
    property color  _aiAccent:      "#2563EB"
    property color  _aiAccentText:  "#FFFFFF"
    property color  _userBubble:    _aiAccent
    property color  _userText:      "#FFFFFF"
    property color  _aiBubble:      _aiDark ? "#2B2F36" : "#EEF1F5"
    property color  _aiText:        _aiDark ? "#E8EAED" : "#1F2329"
    property color  _toolBubble:    _aiDark ? "#23262B" : "#F4F6F8"
    property color  _aiBorder:      _aiDark ? "#3A3F46" : "#DDE1E6"

    // Chat bubble delegate for the AI conversation ListView
    Component {
        id: aiBubbleDelegate

        Item {
            width:  ListView.view ? ListView.view.width : 0
            height: bubbleLoader.height

            property string _role:   modelData.role     || ""
            property bool   _isUser:  _role === "user"
            property bool   _isError: _role === "error"
            property bool   _isStatus: _role === "status"
            property bool   _isTool:  _role === "tool" || _role === "toolResult"
            property real   _maxBubble:    width * 0.78
            property real   _bubblePad:    ScreenTools.defaultFontPixelHeight * 0.65
            property real   _bubbleRadius: ScreenTools.defaultFontPixelHeight * 0.7

            Loader {
                id:             bubbleLoader
                width:          parent.width
                sourceComponent: _isStatus ? statusComp : (_isTool ? toolComp : chatComp)
            }

            // __AI_BUBBLE_SUBCOMP__
            // Centered status line (e.g. "Stopped")
            Component {
                id: statusComp
                Item {
                    width:  bubbleLoader.width
                    height: statusLabel.height + ScreenTools.defaultFontPixelHeight * 0.4
                    QGCLabel {
                        id:                     statusLabel
                        anchors.centerIn:       parent
                        text:                   modelData.text || ""
                        opacity:                0.5
                        font.pointSize:         ScreenTools.smallFontPointSize
                    }
                }
            }

            // User / assistant / error chat bubble
            Component {
                id: chatComp
                Row {
                    width:          bubbleLoader.width
                    layoutDirection: _isUser ? Qt.RightToLeft : Qt.LeftToRight

                    Rectangle {
                        width:      Math.min(_maxBubble, bubbleText.contentWidth + _bubblePad * 2)
                        height:     bubbleText.contentHeight + _bubblePad * 2
                        radius:     _bubbleRadius
                        color:      _isUser   ? _userBubble
                                  : _isError  ? qgcPal.colorRed
                                              : _aiBubble

                        TextEdit {
                            id:                 bubbleText
                            x:                  _bubblePad
                            y:                  _bubblePad
                            width:              Math.min(_maxBubble - _bubblePad * 2, implicitWidth)
                            text:               modelData.text || ""
                            textFormat:         _isUser || _isError ? TextEdit.PlainText : TextEdit.MarkdownText
                            wrapMode:           Text.Wrap
                            readOnly:           true
                            selectByMouse:      true
                            color:              _isUser ? _userText
                                              : _isError ? "white" : _aiText
                            font.pointSize:     ScreenTools.defaultFontPointSize
                            font.family:        ScreenTools.normalFontFamily
                            onLinkActivated:    (link) => Qt.openUrlExternally(link)
                        }
                    }
                }
            }

            // Collapsible tool-call / tool-result card
            Component {
                id: toolComp
                Row {
                    width:          bubbleLoader.width
                    Rectangle {
                        width:      _maxBubble
                        height:     toolCol.height + ScreenTools.defaultFontPixelHeight * 0.6
                        radius:     ScreenTools.defaultFontPixelHeight * 0.4
                        color:      _toolBubble
                        border.color: _aiBorder
                        border.width: 1

                        Column {
                            id:                 toolCol
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            anchors.top:        parent.top
                            anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.8
                            spacing:            ScreenTools.defaultFontPixelHeight * 0.2

                            property bool expanded: false

                            Row {
                                width:      parent.width
                                spacing:    ScreenTools.defaultFontPixelWidth * 0.5

                                QGCColoredImage {
                                    width:      ScreenTools.defaultFontPixelHeight * 0.9
                                    height:     width
                                    source:     _role === "toolResult" ? "/AIChat/terminal.svg" : "/AIChat/wrench.svg"
                                    color:      _role === "toolResult" ? qgcPal.colorGreen : qgcPal.colorBlue
                                    fillMode:   Image.PreserveAspectFit
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                QGCLabel {
                                    width:          parent.width - ScreenTools.defaultFontPixelHeight * 1.4
                                    text:           _role === "toolResult"
                                                    ? qsTr("Result")
                                                    : ((modelData.toolName || "") + "(" + (modelData.args || "") + ")")
                                    elide:          toolCol.expanded ? Text.ElideNone : Text.ElideRight
                                    wrapMode:       toolCol.expanded ? Text.WrapAnywhere : Text.NoWrap
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    font.family:    ScreenTools.fixedFontFamily
                                    opacity:        0.9
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            QGCLabel {
                                width:          parent.width
                                visible:        toolCol.expanded && _role === "toolResult"
                                text:           modelData.result || ""
                                wrapMode:       Text.WrapAnywhere
                                font.pointSize: ScreenTools.smallFontPointSize
                                font.family:    ScreenTools.fixedFontFamily
                                opacity:        0.8
                            }
                        }

                        MouseArea {
                            anchors.fill:   parent
                            onClicked:      toolCol.expanded = !toolCol.expanded
                        }
                    }
                }
            }

        }
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
                    height: ScreenTools.defaultFontPixelHeight * 2
                    color:  qgcPal.windowShade
                    radius: ScreenTools.defaultFontPixelHeight / 4

                    Rectangle {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.bottom: parent.bottom
                        height:         1
                        color:          qgcPal.groupBorder
                    }

                    RowLayout {
                        anchors.fill:           parent
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * 0.6
                        spacing:                ScreenTools.defaultFontPixelWidth * 0.6

                        Rectangle {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 1.5
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                            radius:                 width / 2
                            color:                  _aiAccent
                            QGCColoredImage {
                                anchors.centerIn:   parent
                                width:              parent.width * 0.62
                                height:             width
                                source:             "/AIChat/sparkles.svg"
                                color:              _aiAccentText
                                fillMode:           Image.PreserveAspectFit
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            text:               qsTr("AI Assistant")
                            font.bold:          true
                        }

                        // Clear conversation
                        Rectangle {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 1.6
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.6
                            radius:                 ScreenTools.defaultFontPixelHeight * 0.3
                            color:                  clearHover.containsMouse ? qgcPal.windowShadeLight : "transparent"
                            QGCColoredImage {
                                anchors.centerIn:   parent
                                width:              ScreenTools.defaultFontPixelHeight * 0.95
                                height:             width
                                source:             "/AIChat/trash.svg"
                                color:              qgcPal.text
                                fillMode:           Image.PreserveAspectFit
                            }
                            MouseArea {
                                id:             clearHover
                                anchors.fill:   parent
                                hoverEnabled:   true
                                onClicked:      if (_aiChat) _aiChat.clearConversation()
                            }
                        }

                        // Config toggle
                        Rectangle {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 1.6
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.6
                            radius:                 ScreenTools.defaultFontPixelHeight * 0.3
                            color:                  cfgHover.containsMouse ? qgcPal.windowShadeLight : "transparent"
                            QGCColoredImage {
                                anchors.centerIn:   parent
                                width:              ScreenTools.defaultFontPixelHeight * 0.95
                                height:             width
                                source:             "/AIChat/settings.svg"
                                color:              _aiConfigVisible ? qgcPal.colorGreen : qgcPal.text
                                fillMode:           Image.PreserveAspectFit
                            }
                            MouseArea {
                                id:             cfgHover
                                anchors.fill:   parent
                                hoverEnabled:   true
                                onClicked:      _aiConfigVisible = !_aiConfigVisible
                            }
                        }

                        // Close
                        Rectangle {
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 1.6
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.6
                            radius:                 ScreenTools.defaultFontPixelHeight * 0.3
                            color:                  closeHover.containsMouse ? qgcPal.windowShadeLight : "transparent"
                            QGCColoredImage {
                                anchors.centerIn:   parent
                                width:              ScreenTools.defaultFontPixelHeight * 0.95
                                height:             width
                                source:             "/AIChat/close.svg"
                                color:              qgcPal.text
                                fillMode:           Image.PreserveAspectFit
                            }
                            MouseArea {
                                id:             closeHover
                                anchors.fill:   parent
                                hoverEnabled:   true
                                onClicked:      _aiPanelVisible = false
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
                        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.5
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.4
                        leftPadding:        ScreenTools.defaultFontPixelWidth * 1.2
                        rightPadding:       ScreenTools.defaultFontPixelWidth * 1.2

                        QGCLabel {
                            text:           qsTr("Connection")
                            font.bold:      true
                            font.pointSize: ScreenTools.smallFontPointSize
                            color:          _aiAccent
                        }
                        QGCLabel {
                            text:           qsTr("Base URL")
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                        QGCTextField {
                            width:          parent.width - ScreenTools.defaultFontPixelWidth * 2.4
                            text:           _aiChat ? _aiChat.baseUrl : ""
                            onEditingFinished: if (_aiChat) _aiChat.baseUrl = text
                        }
                        QGCLabel {
                            text:           qsTr("API Key")
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                        QGCTextField {
                            width:          parent.width - ScreenTools.defaultFontPixelWidth * 2.4
                            echoMode:       TextInput.Password
                            text:           _aiChat ? _aiChat.apiKey : ""
                            onEditingFinished: if (_aiChat) _aiChat.apiKey = text
                        }
                        QGCLabel {
                            text:           qsTr("Model")
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                        QGCTextField {
                            width:          parent.width - ScreenTools.defaultFontPixelWidth * 2.4
                            text:           _aiChat ? _aiChat.model : ""
                            onEditingFinished: if (_aiChat) _aiChat.model = text
                        }

                        QGCLabel {
                            text:               qsTr("Permissions")
                            font.bold:          true
                            font.pointSize:     ScreenTools.smallFontPointSize
                            color:              _aiAccent
                            topPadding:         ScreenTools.defaultFontPixelHeight * 0.3
                        }

                        QGCCheckBox {
                            text:               qsTr("Allow AI to control the vehicle")
                            checked:            _aiChat ? _aiChat.vehicleControlEnabled : false
                            onClicked:          if (_aiChat) _aiChat.vehicleControlEnabled = checked
                        }
                        QGCCheckBox {
                            text:               qsTr("Allow AI to change QGC settings")
                            checked:            _aiChat ? _aiChat.settingsControlEnabled : false
                            onClicked:          if (_aiChat) _aiChat.settingsControlEnabled = checked
                        }
                        QGCCheckBox {
                            text:               qsTr("Allow AI to change flight controller parameters (high risk)")
                            checked:            _aiChat ? _aiChat.parameterControlEnabled : false
                            onClicked:          if (_aiChat) _aiChat.parameterControlEnabled = checked
                        }
                        QGCCheckBox {
                            text:               qsTr("Confirm before each command")
                            visible:            _aiChat && (_aiChat.vehicleControlEnabled || _aiChat.settingsControlEnabled || _aiChat.parameterControlEnabled)
                            checked:            _aiChat ? _aiChat.confirmActions : true
                            onClicked:          if (_aiChat) _aiChat.confirmActions = checked
                        }
                    }
                }
                // __AI_CHAT_PLACEHOLDER__
                Item {
                    id:     aiChatArea
                    width:  parent.width
                    height: parent.height
                            - ScreenTools.defaultFontPixelHeight * 2
                            - (_aiConfigVisible ? aiConfigColumn.height + ScreenTools.defaultFontPixelWidth : 0)
                            - aiInputBar.height

                    ListView {
                        id:                 aiListView
                        anchors.fill:       parent
                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                        clip:               true
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                        model:              _aiChat ? _aiChat.conversationModel : []
                        cacheBuffer:        10000

                        onCountChanged:     positionViewAtEnd()
                        onContentHeightChanged: positionViewAtEnd()

                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: aiBubbleDelegate
                    }

                    // Empty-state: greeting + suggestion chips (元宝-style)
                    Column {
                        anchors.centerIn:   parent
                        width:              parent.width * 0.9
                        spacing:            ScreenTools.defaultFontPixelHeight * 1.2
                        visible:            aiListView.count === 0

                        QGCLabel {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text:               qsTr("Hi, how can I help?")
                            font.pointSize:     ScreenTools.largeFontPointSize * 1.3
                            font.bold:          true
                        }

                        Flow {
                            width:              parent.width
                            spacing:            ScreenTools.defaultFontPixelWidth
                            // Centered chips
                            anchors.horizontalCenter: parent.horizontalCenter

                            Repeater {
                                model: [
                                    qsTr("What's the aircraft status?"),
                                    qsTr("Why can't it arm?"),
                                    qsTr("Check the battery and GPS"),
                                    qsTr("Explain the current flight mode")
                                ]
                                Rectangle {
                                    width:      chipText.implicitWidth + ScreenTools.defaultFontPixelWidth * 2.2
                                    height:     ScreenTools.defaultFontPixelHeight * 2
                                    radius:     height / 2
                                    color:      chipMouse.containsMouse ? _aiAccent : _toolBubble
                                    border.color: chipMouse.containsMouse ? _aiAccent : _aiBorder
                                    border.width: 1

                                    QGCLabel {
                                        id:                 chipText
                                        anchors.centerIn:   parent
                                        text:               modelData
                                        font.pointSize:     ScreenTools.smallFontPointSize
                                        color:              chipMouse.containsMouse ? _aiAccentText : qgcPal.text
                                    }
                                    MouseArea {
                                        id:             chipMouse
                                        anchors.fill:   parent
                                        hoverEnabled:   true
                                        enabled:        _aiChat && !_aiChat.busy
                                        onClicked: {
                                            if (_aiChat && !_aiChat.busy) _aiChat.sendMessage(modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // __AI_BUSY_PLACEHOLDER__
                Item {
                    width:      parent.width
                    height:     visible ? ScreenTools.defaultFontPixelHeight * 1.6 : 0
                    visible:    _aiChat && _aiChat.busy

                    Row {
                        anchors.left:           parent.left
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 1.5
                        anchors.verticalCenter: parent.verticalCenter
                        spacing:                ScreenTools.defaultFontPixelWidth * 0.4

                        Repeater {
                            model: 3
                            Rectangle {
                                width:      ScreenTools.defaultFontPixelHeight * 0.35
                                height:     width
                                radius:     width / 2
                                color:      qgcPal.text
                                opacity:    0.3
                                SequentialAnimation on opacity {
                                    running:    _aiChat && _aiChat.busy
                                    loops:      Animation.Infinite
                                    PauseAnimation { duration: index * 180 }
                                    NumberAnimation { to: 1.0; duration: 350 }
                                    NumberAnimation { to: 0.3; duration: 350 }
                                    PauseAnimation { duration: (2 - index) * 180 }
                                }
                            }
                        }
                    }
                }

                Item {
                    id:         aiInputBar
                    width:      parent.width
                    height:     inputCard.height + footnote.height + ScreenTools.defaultFontPixelHeight * 0.9

                    function sendCurrent() {
                        if (_aiChat && !_aiChat.busy && aiInput.text.trim() !== "") {
                            _aiChat.sendMessage(aiInput.text)
                            aiInput.clear()
                        }
                    }

                    Rectangle {
                        id:                 inputCard
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.3
                        height:             inputContent.height + ScreenTools.defaultFontPixelHeight * 1.1
                        radius:             ScreenTools.defaultFontPixelHeight * 0.9
                        color:              qgcPal.textField
                        border.color:       aiInput.activeFocus ? _aiAccent : qgcPal.groupBorder
                        border.width:       1

                        Column {
                            id:                 inputContent
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 1.4
                            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * 1.4
                            spacing:            ScreenTools.defaultFontPixelHeight * 0.5

                            // Multi-line text entry
                            Flickable {
                                width:              parent.width
                                height:             Math.min(aiInput.implicitHeight, ScreenTools.defaultFontPixelHeight * 6)
                                contentHeight:      aiInput.implicitHeight
                                clip:               true

                                TextArea.flickable: TextArea {
                                    id:                 aiInput
                                    wrapMode:           TextArea.Wrap
                                    enabled:            _aiChat && !_aiChat.busy
                                    placeholderText:    qsTr("Ask the AI assistant something...")
                                    color:              qgcPal.textFieldText
                                    placeholderTextColor: qgcPal.text
                                    font.pointSize:     ScreenTools.defaultFontPointSize
                                    font.family:        ScreenTools.normalFontFamily
                                    background:         Item {}
                                    leftPadding:        0
                                    rightPadding:       0
                                    topPadding:         0
                                    Keys.onReturnPressed: (event) => {
                                        if (event.modifiers & Qt.ShiftModifier) {
                                            event.accepted = false
                                        } else {
                                            event.accepted = true
                                            aiInputBar.sendCurrent()
                                        }
                                    }
                                }
                            }

                            // Bottom row: hint left, send button right
                            Item {
                                width:      parent.width
                                height:     sendCircle.height

                                QGCLabel {
                                    anchors.left:           parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    text:                   qsTr("Shift+Enter for newline")
                                    font.pointSize:         ScreenTools.smallFontPointSize
                                    opacity:                0.4
                                }

                                Rectangle {
                                    id:                 sendCircle
                                    anchors.right:          parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    width:              ScreenTools.defaultFontPixelHeight * 2
                                    height:             width
                                    radius:             width / 2
                                    property bool _busy: _aiChat && _aiChat.busy
                                    property bool _canSend: _aiChat && !_aiChat.busy && aiInput.text.trim() !== ""
                                    color:              _busy ? qgcPal.colorRed
                                                      : _canSend ? _aiAccent : qgcPal.buttonHighlight
                                    opacity:            (_busy || _canSend) ? 1.0 : 0.4

                                    QGCColoredImage {
                                        anchors.centerIn:   parent
                                        width:              parent.width * 0.5
                                        height:             width
                                        source:             sendCircle._busy ? "/AIChat/stop.svg"
                                                                              : "/AIChat/send.svg"
                                        color:              _aiAccentText
                                        fillMode:           Image.PreserveAspectFit
                                    }

                                    MouseArea {
                                        anchors.fill:   parent
                                        onClicked: {
                                            if (sendCircle._busy) {
                                                if (_aiChat) _aiChat.stop()
                                            } else {
                                                aiInputBar.sendCurrent()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    QGCLabel {
                        id:                     footnote
                        anchors.top:            inputCard.bottom
                        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
                        anchors.horizontalCenter: parent.horizontalCenter
                        text:                   qsTr("AI-generated content, for reference only")
                        font.pointSize:         ScreenTools.smallFontPointSize
                        opacity:                0.4
                    }
                }
            }
        }
    }

    Connections {
        target: _aiChat
        function onConfirmationRequested(callId, toolName, argsText) {
            var detail = argsText === "" ? toolName : (toolName + "(" + argsText + ")")
            QGroundControl.showMessageDialog(
                _root,
                qsTr("AI Action"),
                qsTr("The AI wants to execute:\n\n%1\n\nAllow this?").arg(detail),
                Dialog.Cancel | Dialog.Ok,
                function() { if (_aiChat) _aiChat.confirmToolCall(callId, true) },
                function() { if (_aiChat) _aiChat.confirmToolCall(callId, false) })
        }
    }
}
