import Blockstream.Green
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: constants.s1
    required property SignMessageResolver resolver
    Label {
        text: qsTrId('id_verify_on_device')
    }
    SectionLabel {
        text: qsTrId('id_device')
    }
    RowLayout {
        Layout.fillHeight: false
        spacing: constants.s1
        DeviceImage {
            device: resolver.device
            sourceSize.height: 24
        }
        Label {
            Layout.fillWidth: true
            text: resolver.device.name
        }
    }
    SectionLabel {
        text: qsTrId('id_message')
    }
    Label {
        text: resolver.message
        wrapMode: Text.Wrap
        Layout.maximumWidth: 500
    }
    SectionLabel {
        text: qsTrId('id_message_hash')
    }
    Label {
        text: String(resolver.hash).match(/.{1,8}/g).join(' ')
        wrapMode: Text.Wrap
        Layout.maximumWidth: 500
    }
    SectionLabel {
        text: qsTrId('id_path_used_for_signing')
    }
    Label {
        text: resolver.path
    }
}
