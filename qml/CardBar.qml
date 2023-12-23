import Blockstream.Green
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    signal assetsClicked()
    required property Context context
    required property Account currentAccount

    id: self
    clip: true
    leftPadding: 16
    rightPadding: 16
    topPadding: 0
    bottomPadding: 0
    background: Rectangle {
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.04)
        color: '#161921'
        radius: 8
    }
    contentItem: Flickable {
        id: flickable
        implicitHeight: layout.implicitHeight
        contentWidth: layout.implicitWidth
        RowLayout {
            id: layout
            spacing: 0
            TotalBalanceCard {
                context: self.context
            }
            Separator {
                visible: assets_card.visible
            }
            AssetsCard {
                id: assets_card
                context: self.context
                background: Rectangle {
                    color: '#FFF'
                    opacity: 0.04
                    visible: hover_handler.hovered
                }
                HoverHandler {
                    id: hover_handler
                    margin: 20
                }
                TapHandler {
                    onTapped: self.assetsClicked()
                }
            }
            Separator {
                visible: jade_card.visible
            }
            JadeCard {
                id: jade_card
                context: self.context
            }
            Separator {
            }
            PriceCard {
                context: self.context
                account: self.currentAccount
            }
            Separator {
                visible: fee_rate_card.visible
            }
            FeeRateCard {
                id: fee_rate_card
                context: self.context
            }
        }
    }
    Image {
        source: 'qrc:/svg2/arrow_right.svg'
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        visible: flickable.contentX > 0
        rotation: 180
        opacity: 0.5
        TapHandler {
            onTapped: flickable.flick(2000, 0)
        }
    }
    Image {
        source: 'qrc:/svg2/arrow_right.svg'
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        visible: flickable.contentWidth - flickable.contentX > flickable.width
        opacity: 0.5
        TapHandler {
            onTapped: flickable.flick(-2000, 0)
        }
    }

    component Separator: Rectangle {
        Layout.minimumWidth: 1
        Layout.maximumWidth: 1
        Layout.fillHeight: true
        color: '#FFF'
        opacity: 0.04
    }
}
