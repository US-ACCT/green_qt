import Blockstream.Green 0.1
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQml.Models 2.11

ColumnLayout {
    required property Account account

    readonly property var selectedOutputs: {
        const outputs = []
        for (var i=0; i<selection_model.selectedIndexes.length; ++i) {
            var index = selection_model.selectedIndexes[i]
            var o = output_model.data(index, Qt.UserRole)
            outputs.push(o)
        }
        return outputs;
    }

    id: self
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: constants.p2

    RowLayout {
        id: tags_layout
        Layout.fillWidth: true
        implicitHeight: 50
        spacing: constants.p1

        ButtonGroup {
            id: button_group
        }

        Repeater {
            model: account.wallet.network.liquid ? ['all', 'csv', 'p2wsh', 'not confidential'] : ['all', 'csv', 'p2wsh', 'dust', 'locked']
            delegate: Button {
                id: self
                ButtonGroup.group: button_group
                checked: index === 0
                checkable: true
                background: Rectangle {
                    id: rectangle
                    radius: 4
                    color: self.checked ? constants.c200 : constants.c300
                }
                text: modelData
                ToolTip.delay: 300
                ToolTip.visible: hovered
                ToolTip.text: qsTrId(`id_tag_${modelData}`);
            }
        }

        HSpacer {
        }

        ToolButton {
            icon.source: "qrc:/svg/info.svg"
            icon.color: "white"
            onClicked: info_dialog.createObject(self).open();
        }
    }

    ListView {
        id: list_view
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 8
        model: OutputListModelFilter {
            id: output_model_filter
            filter: button_group.checkedButton.text
            model: OutputListModel {
                id: output_model
                account: self.account
            }
        }
        delegate: OutputDelegate {
            highlighted: selection_model.selectedIndexes.indexOf(output_model.index(output_model.indexOf(output), 0))>-1
            width: list_view.width
        }

        ScrollIndicator.vertical: ScrollIndicator {}

        BusyIndicator {
            width: 32
            height: 32
            running: output_model.fetching
            anchors.margins: 8
            Layout.alignment: Qt.AlignHCenter
            opacity: output_model.fetching ? 1 : 0
            Behavior on opacity { OpacityAnimator {} }
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Label {
            id: label
            visible: list_view.count === 0
            anchors.centerIn: parent
            color: 'white'
            text: {
                if (output_model_filter.filter === '') {
                    return qsTrId(`You'll see your coins here when you receive funds`)
                } else {
                    return qsTrId('There are no results for the applied filter')
                }
            }
        }

        ItemSelectionModel {
            id: selection_model
            model: output_model
        }
    }

    RowLayout {
        visible: selection_model.hasSelection && !account.wallet.network.liquid
        Layout.fillWidth: true
        spacing: constants.p1

        Label {
            text: selection_model.selectedIndexes.length + ' selected'
            padding: 4
        }

        HSpacer {
        }

        GButton {
            text: qsTrId('id_lock')
            enabled: {
                for (const output of selectedOutputs) {
                    if (output.data.satoshi>1092 || output.locked) return false;
                }
                return true;
            }
            onClicked: {
                var outputs = selectedOutputs;
                selection_model.clearSelection()
                set_unspent_outputs_status_dialog.createObject(self, { outputs, status: "frozen" }).open();
            }
        }

        GButton {
            text: qsTrId('id_unlock')
            enabled: {
                for (const output of selectedOutputs) {
                    if (!output.locked) return false;
                }
                return true;
            }
            onClicked: {
                var outputs = selectedOutputs;
                selection_model.clearSelection()
                set_unspent_outputs_status_dialog.createObject(self, { outputs, status: "default" }).open();
            }
        }

        GButton {
            text: qsTrId('id_clear')
            onClicked: selection_model.clear();
        }
    }

    Component {
        id: set_unspent_outputs_status_dialog
        SetUnspentOutputsStatusDialog {
            model: output_model
            wallet: self.account.wallet
        }
    }

    Component {
        id: info_dialog
        AbstractDialog {
            title: qsTrId('id_filter_types')
            icon: "qrc:/svg/info.svg"
            contentItem: ColumnLayout {
                spacing: constants.p1
                Repeater {
                    model: [['ALL', 'Lists all available coins'],
                            ['CSV', 'Lists coins that have CSV address type'],
                            ['P2WSH', 'Lists coins that have P2WSH address type'],
                            ['NOT CONFIDENTIAL', 'Lists coins that are not confidential'],
                            ['DUST', 'Lists coins that have a value < 1092 satoshi'],
                            ['LOCKED', 'Lists coins that are locked']
                            ]
                    delegate: RowLayout {
                        spacing: constants.p1
                        Label {
                            text: modelData[0]
                            padding: 6
                            background: Rectangle {
                                id: rectangle
                                radius: 4
                                color: constants.c300
                            }
                        }

                        Label {
                            text: modelData[1]
                        }
                    }
                }
            }
        }
    }
}
