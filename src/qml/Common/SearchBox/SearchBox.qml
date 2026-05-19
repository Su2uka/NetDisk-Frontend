import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCore
import FluentUI

Item {
    id: root
    height: 36

    // ═══════════════════════════════════════
    //  Public API
    // ═══════════════════════════════════════

    signal searchTriggered(string keyword)
    signal fileSelected(int fileId)
    signal cloudSearchRequested(string keyword)

    function loadCloudResults(jsonArray) {
        _cloudModel.clear();
        for (var i = 0; i < jsonArray.length; i++)
            _cloudModel.append(jsonArray[i]);   // { fileId: int, fileName: string }
    }

    // ═══════════════════════════════════════
    //  State Machine
    // ═══════════════════════════════════════

    state: searchInput.text.length > 0 ? "cloud" : "history"

    states: [
        State {
            name: "history"
            PropertyChanges {
                target: _historyPanel
                visible: true
            }
            PropertyChanges {
                target: _cloudPanel
                visible: false
            }
        },
        State {
            name: "cloud"
            PropertyChanges {
                target: _historyPanel
                visible: false
            }
            PropertyChanges {
                target: _cloudPanel
                visible: true
            }
        }
    ]

    // ═══════════════════════════════════════
    //  Internal Models
    // ═══════════════════════════════════════

    ListModel {
        id: _historyModel
    }     // role: text
    ListModel {
        id: _cloudModel
    }       // roles: fileId, fileName

    // ═══════════════════════════════════════
    //  Debounce Timer (300ms)
    // ═══════════════════════════════════════

    Timer {
        id: _debounce
        interval: 300
        repeat: false
        onTriggered: {
            var kw = searchInput.text.trim();
            if (kw.length > 0)
                root.cloudSearchRequested(kw);
        }
    }

    // ═══════════════════════════════════════
    //  History Persistence (LRU, max 10)
    // ═══════════════════════════════════════

    Settings {
        id: _settings
        category: "SearchBox"
        property string historyData: "[]"
    }

    Component.onCompleted: _loadHistory()
    Component.onDestruction: _saveHistory()

    function _loadHistory() {
        try {
            var arr = JSON.parse(_settings.historyData);
            for (var i = 0; i < Math.min(arr.length, 10); i++)
                _historyModel.append({
                    "text": arr[i]
                });
        } catch (e) {
            console.warn("[SearchBox] history parse error:", e);
        }
    }

    function _saveHistory() {
        var arr = [];
        for (var i = 0; i < _historyModel.count; i++)
            arr.push(_historyModel.get(i).text);
        _settings.historyData = JSON.stringify(arr);
    }

    function _addHistory(keyword) {
        var kw = keyword.trim();
        if (kw.length === 0)
            return;
        for (var i = 0; i < _historyModel.count; i++) {
            if (_historyModel.get(i).text === kw) {
                _historyModel.remove(i);
                break;
            }
        }
        _historyModel.insert(0, {
            "text": kw
        });
        while (_historyModel.count > 10)
            _historyModel.remove(_historyModel.count - 1);
        _saveHistory();
    }

    function _doSearch(keyword) {
        var kw = keyword.trim();
        if (kw.length === 0)
            return;
        _addHistory(kw);
        searchTriggered(kw);
        _popup.close();
    }

    // ═══════════════════════════════════════
    //  Search Bar
    // ═══════════════════════════════════════

    Rectangle {
        id: _bar
        anchors.fill: parent
        radius: height / 2
        color: searchInput.activeFocus ? "#e8e8e8" : "#f0f0f0"
        border.color: searchInput.activeFocus ? "#c0c0c0" : "transparent"
        border.width: 1
        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: 14
                rightMargin: 14
            }
            spacing: 8

            FluIcon {
                iconSource: FluentIcons.Search
                iconSize: 14
                iconColor: "#888888"
                Layout.alignment: Qt.AlignVCenter
            }

            TextInput {
                id: searchInput
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                font.pixelSize: 14
                color: "#333333"
                clip: true
                selectByMouse: true

                Text {
                    anchors.fill: parent
                    text: "搜索"
                    color: "#999999"
                    font.pixelSize: 14
                    verticalAlignment: Text.AlignVCenter
                    visible: !searchInput.text && !searchInput.activeFocus
                }

                onTextEdited: {
                    _debounce.restart();
                    if (!_popup.visible)
                        _popup.open();
                }

                onActiveFocusChanged: {
                    if (activeFocus && !_popup.visible)
                        _popup.open();
                }

                Keys.onReturnPressed: _doSearch(text)
                Keys.onEnterPressed: _doSearch(text)
                Keys.onEscapePressed: {
                    _popup.close();
                    focus = false;
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: searchInput.forceActiveFocus()
            z: -1
        }
    }

    // ═══════════════════════════════════════
    //  Dropdown Popup
    // ═══════════════════════════════════════

    Popup {
        id: _popup
        y: _bar.height + 4
        width: _bar.width
        padding: 0
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
        modal: false
        focus: false

        onClosed: {
            searchInput.focus = false;
        }

        background: Rectangle {
            radius: 8
            color: "#ffffff"
            border.color: "#e0e0e0"
            border.width: 1
        }

        contentItem: Column {
            width: _popup.width

            // ──── History Panel ────
            Column {
                id: _historyPanel
                width: parent.width

                RowLayout {
                    width: parent.width
                    height: 40
                    Item {
                        width: 16
                    }
                    FluText {
                        text: "搜索记录"
                        font.pixelSize: 13
                        color: "#888888"
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    FluTextButton {
                        text: "清空"
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: {
                            _historyModel.clear();
                            root._saveHistory();
                        }
                    }
                    Item {
                        width: 8
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#f0f0f0"
                    visible: _historyModel.count > 0
                }

                ListView {
                    width: parent.width
                    height: Math.min(contentHeight, 240)
                    model: _historyModel
                    clip: true
                    interactive: contentHeight > height
                    visible: _historyModel.count > 0

                    delegate: HistoryDelegate {
                        onSearched: function (keyword) {
                            root._doSearch(keyword);
                        }
                        onDeleted: function (itemIndex) {
                            _historyModel.remove(itemIndex);
                            root._saveHistory();
                        }
                    }
                }

                Item {
                    width: parent.width
                    height: 60
                    visible: _historyModel.count === 0
                    FluText {
                        anchors.centerIn: parent
                        text: "暂无搜索记录"
                        font.pixelSize: 13
                        color: "#bbbbbb"
                    }
                }
            }

            // ──── Cloud Panel ────
            Column {
                id: _cloudPanel
                width: parent.width

                RowLayout {
                    width: parent.width
                    height: 44
                    Item {
                        width: 16
                    }
                    FluText {
                        text: searchInput.text
                        font {
                            pixelSize: 15
                            bold: true
                        }
                        color: "#222222"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                    }
                    FluTextButton {
                        text: "清空"
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: {
                            searchInput.text = "";
                            _cloudModel.clear();
                            searchInput.forceActiveFocus();
                        }
                    }
                    FluIconButton {
                        iconSource: FluentIcons.ChromeClose
                        iconSize: 12
                        width: 30
                        height: 30
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: _popup.close()
                    }
                    Item {
                        width: 4
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#f0f0f0"
                }

                ListView {
                    width: parent.width
                    height: Math.min(contentHeight, 200)
                    model: _cloudModel
                    clip: true
                    interactive: contentHeight > height
                    visible: _cloudModel.count > 0

                    delegate: CloudResultDelegate {
                        onSelected: function (fileId) {
                            root.fileSelected(fileId);
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#f0f0f0"
                }

                // 「查看全部结果」底栏
                Item {
                    width: parent.width
                    height: 48

                    Rectangle {
                        anchors {
                            fill: parent
                            leftMargin: 6
                            rightMargin: 6
                            topMargin: 3
                            bottomMargin: 6
                        }
                        radius: 4
                        color: _footerArea.containsMouse ? "#f5f5f5" : "transparent"
                    }

                    MouseArea {
                        id: _footerArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root._doSearch(searchInput.text)
                    }

                    RowLayout {
                        anchors {
                            fill: parent
                            leftMargin: 16
                            rightMargin: 16
                        }
                        spacing: 8

                        FluIcon {
                            iconSource: FluentIcons.Search
                            iconSize: 14
                            iconColor: "#888888"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        FluText {
                            text: "查看"
                            font.pixelSize: 13
                            color: "#666666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        FluText {
                            text: searchInput.text
                            font {
                                pixelSize: 13
                                bold: true
                            }
                            color: "#0078d4"
                            elide: Text.ElideRight
                            Layout.maximumWidth: 200
                            Layout.alignment: Qt.AlignVCenter
                        }
                        FluText {
                            text: "的所有搜索结果"
                            font.pixelSize: 13
                            color: "#666666"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        FluText {
                            text: "Shift+Enter"
                            font.pixelSize: 12
                            color: "#aaaaaa"
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }
            }
        }
    }
}
