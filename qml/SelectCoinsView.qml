import Blockstream.Green
import Blockstream.Green.Core
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

import "analytics.js" as AnalyticsJS

StackViewPage {
    signal coinsSelected(var coins)
    required property Account account
    required property Asset asset
    required property var coins
    required property string unit

    property var selection: {
        const selection = new Set()
        for (let i = 0; i < self.coins.length; i++) {
            selection.add(self.coins[i])
        }
        return selection
    }

    id: self
    title: qsTrId('id_select_your_coins')
    contentItem: ColumnLayout {
        RowLayout {
            Layout.alignment: Qt.AlignCenter
            Layout.bottomMargin: 20
            Layout.fillWidth: false
            spacing: 10

            ButtonGroup {
                id: button_group
            }

            Repeater {
                model: {
                    const filters = ['', 'csv', 'p2wsh']
                    if (account.network.liquid) {
                        filters.push('not_confidential')
                    } else {
                        filters.push('p2sh')
                        filters.push('dust')
                    }
                    if (account.type !== '2of3' && account.type !== '2of2_no_recovery') {
                        filters.push('expired')
                    }
                    return filters
                }
                delegate: AbstractButton {
                    id: self
                    ButtonGroup.group: button_group
                    checked: index === 0
                    checkable: true
                    background: null
                    opacity: self.checked ? 1 : 0.4
                    contentItem: Label {
                        text: self.text
                        font.pixelSize: 12
                        font.weight: 400
                        font.styleName: 'Regular'
                    }
                    text: localizedLabel(modelData)
                    property string buttonTag: modelData
                    font.capitalization: Font.AllUppercase
                }
            }
        }

        TListView {
            id: list_view
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 5
            model: OutputListModelFilter {
                id: output_model_filter
                filter: [button_group.checkedButton?.buttonTag ?? '', '!locked'].join(' ')
                asset: self.asset
                model: OutputListModel {
                    id: output_model
                    account: self.account
                }
            }
            delegate: CoinDelegate {
                id: delegate
                checked: self.selection.has(delegate.output)
                width: ListView.view.width
                unit: self.unit
                onClicked: {
                    const output = delegate.output
                    const selection = new Set(self.selection)
                    if (self.selection.has(output)) {
                        selection.delete(output)
                    } else {
                        selection.add(output)
                    }
                    self.selection = selection
                }
            }

            Label {
                id: label
                visible: list_view.count === 0
                anchors.centerIn: parent
                color: 'white'
                text: {
                    if (output_model_filter.filter === '') {
                        return qsTrId('id_youll_see_your_coins_here_when')
                    } else {
                        return qsTrId('id_there_are_no_results_for_the')
                    }
                }
            }
        }
    }
    footerItem: ColumnLayout {
        spacing: 10
        PrimaryButton {
            Layout.fillWidth: true
            enabled: self.selection.size > 0
            text: qsTrId('id_confirm_coin_selection')
            onClicked: {
                const coins = []
                for (const output of self.selection) {
                    coins.push(output)
                }
                self.coinsSelected(coins)
            }
        }
        Label {
            Layout.alignment: Qt.AlignCenter
            font.pixelSize: 12
            font.weight: 500
            text: qsTrId('id_d_coins_selected').arg(self.selection.size)
            visible: self.selection.size > 0
        }
        Label {
            Layout.alignment: Qt.AlignCenter
            font.pixelSize: 14
            font.weight: 400
            text: qsTrId('id_you_can_send_up_to')
            visible: self.selection.size > 0
        }
        Label {
            Layout.alignment: Qt.AlignCenter
            font.pixelSize: 16
            font.weight: 400
            text: convert.output.label
            visible: self.selection.size > 0
        }
        Label {
            Layout.alignment: Qt.AlignCenter
            font.pixelSize: 16
            font.weight: 400
            opacity: 0.6
            text: convert.fiat.label
            visible: self.selection.size > 0
        }
        Convert {
            id: convert
            account: self.account
            asset: self.asset
            input: {
                let satoshi = 0
                for (const output of self.selection) {
                    satoshi += output.data.satoshi
                }
                return { satoshi }
            }
            unit: self.unit
        }
    }

    AnalyticsView {
        active: self.visible
        name: 'SelectUTXO'
        segmentation: AnalyticsJS.segmentationSubAccount(Settings, self.account)
    }
}
