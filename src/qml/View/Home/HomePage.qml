import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI
import App 1.0

Item {
    id: root

    // ── StackView：首页 ↔ 分类详情 ──
    StackView {
        id: homeStack
        anchors.fill: parent
        initialItem: homeContent

        // 禁用所有过渡动画
        pushEnter: null
        pushExit: null
        popEnter: null
        popExit: null
        replaceEnter: null
        replaceExit: null
    }

    // ── 首页主内容 ──
    Component {
        id: homeContent

        FluScrollablePage {

            StorageCard {
                Layout.fillWidth: true
                Layout.topMargin: 10
                Layout.leftMargin: 30
                Layout.rightMargin: 35
            }

            SmartCategories {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 30
                Layout.rightMargin: 35

                onCategoryClicked: function (categoryName, categoryKey) {
                    homeStack.push(categoryPage, {
                        categoryName: categoryName,
                        categoryKey: categoryKey
                    });
                }
            }

            RecentFiles {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 30
                Layout.rightMargin: 35
                Layout.bottomMargin: 12
            }
        }
    }

    // ── 分类详情页 ──
    Component {
        id: categoryPage

        CategoryDetailPage {
            onGoBack: {
                homeStack.pop();
                // 回到首页后恢复文件列表为当前目录
                FileController.loadFiles();
            }
        }
    }
}
