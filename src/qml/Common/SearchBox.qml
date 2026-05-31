import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCore
import FluentUI
import App 1.0

Item {
    id: root
    height: 36

    // ──── Public API ────
    signal fileSelected(var fileInfo)
    signal cloudSearchRequested(string keyword)
    property alias keyword: searchInput.text
    property bool cloudLoading: false

    function loadCloudResults(jsonArray) {
        cloudLoading = false;
        _cloudModel.clear();
        for (var i = 0; i < jsonArray.length; i++)
            _cloudModel.append(jsonArray[i]);   // { fileId: int, fileName: string }
    }

    function setCloudLoading(loading) {
        cloudLoading = loading;
    }

    // ──── Panel Visibility ────
    readonly property bool _isCloudMode: searchInput.text.length > 0

    // ──── Internal Models ────
    ListModel {
        id: _historyModel
    }     // role: text
    ListModel {
        id: _cloudModel
    }       // roles: fileId, fileName

    // ──── Debounce Timer (300ms) ────
    Timer {
        id: _debounce
        interval: 300
        repeat: false
        onTriggered: {
            var kw = searchInput.text.trim();
            if (kw.length > 0) {
                root.setCloudLoading(true);
                root.cloudSearchRequested(kw);
            }
        }
    }

    // ──── History Persistence (LRU, max 10) ────
    Settings {
        id: _settings
        category: UserController.userId > 0 ? "Users/user_" + UserController.userId + "/SearchBox" : "Users/anonymous/SearchBox"
        property string historyData: "[]"
    }

    Component.onCompleted: _loadHistory()
    Component.onDestruction: _saveHistory()

    Connections {
        target: UserController
        function onUserInfoChanged() {
            _loadHistory();
        }
    }

    function _loadHistory() {
        try {
            _historyModel.clear();
            var raw = _settings.historyData;
            console.log("[SearchBox] _loadHistory raw:", raw);
            var arr = JSON.parse(raw);
            for (var i = 0; i < Math.min(arr.length, 10); i++)
                _historyModel.append({
                    "text": arr[i]
                });
            console.log("[SearchBox] loaded", _historyModel.count, "history items");
        } catch (e) {
            console.warn("[SearchBox] history parse error:", e);
        }
    }

    function _saveHistory() {
        var arr = [];
        for (var i = 0; i < _historyModel.count; i++)
            arr.push(_historyModel.get(i).text);
        _settings.historyData = JSON.stringify(arr);
        _settings.sync();
        console.log("[SearchBox] saved", arr.length, "history items:", _settings.historyData);
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

    function selectCloudResult(item) {
        var info = {
            fileId: item.fileId,
            fileName: item.fileName,
            isFolder: item.isFolder,
            parentId: item.parentId
        };
        _addHistory(searchInput.text);
        fileSelected(info);
        _popup.close();
    }

    function activateCurrentItem() {
        if (searchInput.text.length > 0 && cloudList.currentIndex >= 0 && cloudList.currentIndex < _cloudModel.count) {
            selectCloudResult(_cloudModel.get(cloudList.currentIndex));
            return true;
        }
        if (searchInput.text.length === 0 && historyList.currentIndex >= 0 && historyList.currentIndex < _historyModel.count) {
            searchInput.text = _historyModel.get(historyList.currentIndex).text;
            searchInput.forceActiveFocus();
            root.setCloudLoading(true);
            root.cloudSearchRequested(searchInput.text.trim());
            return true;
        }
        return false;
    }

    // ──── Search Bar ────
    Rectangle {
        id: _bar
        anchors.fill: parent
        radius: height / 2
        color: searchInput.activeFocus ? (FluTheme.dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.04)) : (FluTheme.dark ? Qt.rgba(1, 1, 1, 0.03) : Qt.rgba(0, 0, 0, 0.02))
        border.color: searchInput.activeFocus ? FluTheme.primaryColor : "transparent"
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
                rightMargin: 10
            }
            spacing: 8

            FluIcon {
                iconSource: FluentIcons.Search
                iconSize: 14
                iconColor: FluTheme.fontTertiaryColor
                Layout.alignment: Qt.AlignVCenter
            }

            TextField {
                id: searchInput
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                font.pixelSize: 14
                color: FluTheme.fontPrimaryColor
                placeholderTextColor: FluTheme.fontSecondaryColor
                placeholderText: "搜索"
                selectByMouse: true
                background: Item {}
                padding: 0

                onTextEdited: {
                    _debounce.restart();
                    if (!_popup.visible)
                        _popup.open();
                }

                onActiveFocusChanged: {
                    if (activeFocus && !_popup.visible)
                        _popup.open();
                }

                Keys.onReturnPressed: root.activateCurrentItem()
                Keys.onEnterPressed: root.activateCurrentItem()
                Keys.onEscapePressed: {
                    _popup.close();
                    focus = false;
                }
                Keys.onUpPressed: {
                    if (_popup.visible) {
                        var list = searchInput.text.length > 0 ? cloudList : historyList;
                        if (list.count > 0) {
                            if (list.currentIndex > 0) {
                                list.currentIndex--;
                            } else {
                                list.currentIndex = list.count - 1;
                            }
                        }
                    }
                }
                Keys.onDownPressed: {
                    if (_popup.visible) {
                        var list = searchInput.text.length > 0 ? cloudList : historyList;
                        if (list.count > 0) {
                            if (list.currentIndex < list.count - 1) {
                                list.currentIndex++;
                            } else {
                                list.currentIndex = 0;
                            }
                        }
                    }
                }
            }

            // Clear Button
            FluIconButton {
                iconSource: FluentIcons.ChromeClose
                iconSize: 10
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                Layout.alignment: Qt.AlignVCenter
                visible: searchInput.text.length > 0
                onClicked: {
                    searchInput.text = "";
                    root.setCloudLoading(false);
                    _cloudModel.clear();
                    searchInput.forceActiveFocus();
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: searchInput.forceActiveFocus()
            z: -1
        }
    }

    // ──── Dropdown Popup ────
    Popup {
        id: _popup
        y: _bar.height + 4
        width: _bar.width
        contentHeight: _popupColumn.implicitHeight
        padding: 0
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
        modal: false
        focus: false

        enter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 150 }
                NumberAnimation { property: "y"; from: _bar.height - 4; to: _bar.height + 4; duration: 150; easing.type: Easing.OutQuad }
            }
        }

        exit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 150 }
                NumberAnimation { property: "y"; from: _bar.height + 4; to: _bar.height - 4; duration: 150; easing.type: Easing.InQuad }
            }
        }

        onOpened: {
            historyList.currentIndex = -1;
            cloudList.currentIndex = -1;
        }

        onClosed: {
            searchInput.focus = false;
        }

        background: Item {
            FluShadow {
                radius: 8
            }
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: FluTheme.dark ? "#2d2d2d" : "#ffffff"
                border.color: FluTheme.dividerColor
                border.width: 1
            }
        }

        contentItem: Column {
            id: _popupColumn
            width: _popup.width

            // History Panel
            Column {
                id: _historyPanel
                visible: !root._isCloudMode
                width: parent.width

                RowLayout {
                    width: parent.width
                    height: 40
                    Item { width: 16 }
                    FluText {
                        text: "搜索记录"
                        font.pixelSize: 13
                        color: FluTheme.fontTertiaryColor
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Item { Layout.fillWidth: true }
                    FluTextButton {
                        text: "清空"
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: {
                            _historyModel.clear();
                            root._saveHistory();
                        }
                    }
                    Item { width: 8 }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: FluTheme.dividerColor
                    visible: _historyModel.count > 0
                }

                ListView {
                    id: historyList
                    width: parent.width
                    height: Math.min(contentHeight, 240)
                    model: _historyModel
                    clip: true
                    interactive: contentHeight > height
                    visible: _historyModel.count > 0
                    keyNavigationEnabled: true
                    currentIndex: -1

                    delegate: Item {
                        width: ListView.view.width
                        height: 44

                        property int itemIndex: index
                        property int listCurrentIndex: ListView.view.currentIndex

                        Rectangle {
                            anchors {
                                fill: parent
                                leftMargin: 6
                                rightMargin: 6
                                topMargin: 3
                                bottomMargin: 3
                            }
                            radius: 4
                            color: historyItemMouseArea.containsMouse || itemIndex === listCurrentIndex
                                   ? (FluTheme.dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.03))
                                   : "transparent"
                        }

                        MouseArea {
                            id: historyItemMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: { searchInput.text = model.text; searchInput.forceActiveFocus(); root.setCloudLoading(true); root.cloudSearchRequested(model.text.trim()); }
                        }

                        RowLayout {
                            anchors {
                                fill: parent
                                leftMargin: 16
                                rightMargin: 12
                            }
                            spacing: 12

                            FluIcon {
                                iconSource: FluentIcons.History
                                iconSize: 16
                                iconColor: FluTheme.fontTertiaryColor
                                Layout.alignment: Qt.AlignVCenter
                            }

                            FluText {
                                text: model.text
                                font.pixelSize: 14
                                color: FluTheme.fontPrimaryColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }

                            FluIconButton {
                                iconSource: FluentIcons.ChromeClose
                                iconSize: 10
                                iconColor: FluTheme.fontTertiaryColor
                                width: 28
                                height: 28
                                Layout.alignment: Qt.AlignVCenter
                                onClicked: {
                                    _historyModel.remove(index);
                                    root._saveHistory();
                                }
                            }
                        }

                        Keys.onReturnPressed: { searchInput.text = model.text; searchInput.forceActiveFocus(); root.setCloudLoading(true); root.cloudSearchRequested(model.text.trim()); }
                        Keys.onEnterPressed: { searchInput.text = model.text; searchInput.forceActiveFocus(); root.setCloudLoading(true); root.cloudSearchRequested(model.text.trim()); }
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
                        color: FluTheme.fontTertiaryColor
                    }
                }
            }

            // Cloud Panel
            Column {
                id: _cloudPanel
                visible: root._isCloudMode
                width: parent.width

                ListView {
                    id: cloudList
                    width: parent.width
                    height: Math.min(contentHeight, 200)
                    model: _cloudModel
                    clip: true
                    interactive: contentHeight > height
                    visible: _cloudModel.count > 0
                    keyNavigationEnabled: true
                    currentIndex: -1

                    delegate: Item {
                        width: ListView.view.width
                        height: 44

                        property int itemIndex: index
                        property int listCurrentIndex: ListView.view.currentIndex

                        Rectangle {
                            anchors {
                                fill: parent
                                leftMargin: 6
                                rightMargin: 6
                                topMargin: 3
                                bottomMargin: 3
                            }
                            radius: 4
                            color: cloudItemMouseArea.containsMouse || itemIndex === listCurrentIndex
                                   ? (FluTheme.dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.03))
                                   : "transparent"
                        }

                        MouseArea {
                            id: cloudItemMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.selectCloudResult(model)
                        }

                        RowLayout {
                            anchors {
                                fill: parent
                                leftMargin: 16
                                rightMargin: 16
                            }
                            spacing: 12

                            FluIcon {
                                iconSource: FluentIcons.OpenFile
                                iconSize: 16
                                iconColor: FluTheme.fontTertiaryColor
                                Layout.alignment: Qt.AlignVCenter
                            }

                            FluText {
                                text: model.fileName
                                font.pixelSize: 14
                                color: FluTheme.fontPrimaryColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        Keys.onReturnPressed: root.selectCloudResult(model)
                        Keys.onEnterPressed: root.selectCloudResult(model)
                    }
                }

                Item {
                    width: parent.width
                    height: 56
                    visible: root.cloudLoading || _cloudModel.count === 0

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: root.cloudLoading
                        visible: root.cloudLoading
                        width: 22
                        height: 22
                    }

                    FluText {
                        anchors.centerIn: parent
                        visible: !root.cloudLoading && _cloudModel.count === 0
                        text: "暂无匹配结果"
                        font.pixelSize: 13
                        color: FluTheme.fontTertiaryColor
                    }
                }
            }
        }
    }
}
