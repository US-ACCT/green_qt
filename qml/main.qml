import Blockstream.Green
import Blockstream.Green.Core
import QtMultimedia
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

import "analytics.js" as AnalyticsJS
import "util.js" as UtilJS

ApplicationWindow {
    readonly property WalletView currentWalletView: stack_layout.currentWalletView
    readonly property Wallet currentWallet: currentWalletView?.wallet ?? null
    readonly property Account currentAccount: currentWalletView?.currentAccount ?? null

    property Navigation navigation: Navigation {}
    property Constants constants: Constants {}

    id: window
    x: Settings.windowX
    y: Settings.windowY
    width: Settings.windowWidth
    height: Settings.windowHeight
    onXChanged: Settings.windowX = x
    onYChanged: Settings.windowY = y
    onWidthChanged: Settings.windowWidth = width
    onHeightChanged: Settings.windowHeight = height
    onCurrentWalletChanged: {
        if (currentWallet?.persisted) {
            Settings.updateRecentWallet(currentWallet.id)
        }
    }
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    color: constants.c800
    title: {
        const parts = env === 'Development' ? [navigation.description] : []
        if (currentWallet) {
            parts.push(font_metrics.elidedText(currentWallet.name, Qt.ElideRight, window.width / 3));
            if (currentAccount) parts.push(font_metrics.elidedText(UtilJS.accountName(currentAccount), Qt.ElideRight, window.width / 3));
        }
        parts.push('Blockstream Green');
        if (env !== 'Production') parts.push(`[${env}]`)
        return parts.join(' - ');
    }
    Label {
        parent: Overlay.overlay
        visible: false
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 4
        z: 1000
        HoverHandler {
            id: debug_focus_hover_handler
        }
        opacity: debug_focus_hover_handler.hovered ? 1.0 : 0.3
        background: Rectangle {
            color: Qt.rgba(1, 0, 0, 0.8)
            radius: 4
            border.width: 2
            border.color: 'black'
        }
        padding: 8
        text: {
            const parts = []
            let item = activeFocusItem
            while (item) {
                parts.unshift((''+item).split('(')[0])
                item = item.parent
            }
            return parts.join(' > ')
        }
    }
    FontMetrics {
        id: font_metrics
    }
    RowLayout {
        id: main_layout
        anchors.fill: parent
        spacing: 0
        SideBar {
            Layout.fillHeight: true
        }
        StackLayout {
            id: stack_layout
            Layout.fillWidth: true
            Layout.fillHeight: true
            readonly property WalletView currentWalletView: currentIndex < 0 ? null : (stack_layout.children[currentIndex].currentWalletView || null)
            currentIndex: UtilJS.findChildIndex(stack_layout, child => child instanceof Item && child.active && child.enabled)
            HomeView {
                readonly property bool active: navigation.param.view === 'home'
                focus: StackLayout.isCurrentItem
            }
            BlockstreamView {
                readonly property bool active: navigation.param.view === 'blockstream'
                id: blockstream_view
                focus: StackLayout.isCurrentItem
            }
            OnboardView {
                readonly property bool active: !navigation.param.view || navigation.param.view === 'onboard'
                focus: StackLayout.isCurrentItem
            }
            PreferencesView {
                readonly property bool active: navigation.param.view === 'preferences'
                focus: StackLayout.isCurrentItem
            }
            JadeView {
                readonly property bool active: navigation.param.view === 'jade'
                id: jade_view
                focus: StackLayout.isCurrentItem
            }
            LedgerDevicesView {
                readonly property bool active: navigation.param.view === 'ledger'
                id: ledger_view
                focus: StackLayout.isCurrentItem
            }
            NetworkView {
                title: qsTrId('id_wallets')
                focus: StackLayout.isCurrentItem
            }
        }
    }

    AnalyticsConsentDialog {
        property real offset_y
        x: parent.width - width - constants.s2
        y: parent.height - height - constants.s2 - 30 + offset_y
        visible: Settings.analytics === ''
        enter: Transition {
            SequentialAnimation {
                PropertyAction { property: 'x'; value: 0 }
                PropertyAction { property: 'offset_y'; value: 100 }
                PropertyAction { property: 'opacity'; value: 0 }
                PauseAnimation { duration: 2000 }
                ParallelAnimation {
                    NumberAnimation { property: 'opacity'; to: 1; easing.type: Easing.OutCubic; duration: 1000 }
                    NumberAnimation { property: 'offset_y'; to: 0; easing.type: Easing.OutCubic; duration: 1000 }
                }
            }
        }
    }

    Loader2 {
        active: navigation.param.flow === 'signup'
        onActiveChanged: if (!active) object.close()
        sourceComponent: SignupDialog {
            visible: true
            onRejected: navigation.pop()
            onClosed: destroy()
        }
    }

    Loader2 {
        active: navigation.param.flow === 'restore'
        onActiveChanged: if (!active) object.close()
        sourceComponent: RestoreDialog {
            visible: true
            onRejected: navigation.pop()
            onClosed: destroy()
        }
    }

    Loader2 {
        active: navigation.param.flow === 'watch_only_login'
        onActiveChanged: if (!active) object.close()
        sourceComponent: WatchOnlyLoginDialog {
            network: NetworkManager.networkWithServerType(navigation.param.network, 'green')
            visible: true
            onRejected: navigation.pop()
            onClosed: destroy()
        }
    }

    Component {
        id: create_account_dialog
        CreateAccountDialog {}
    }

    Component {
        id: remove_wallet_dialog
        RemoveWalletDialog {}
    }

    readonly property bool scannerAvailable: (media_devices.item?.videoInputs?.length > 0) ?? false
    Loader {
        id: media_devices
        asynchronous: true
        active: true
        sourceComponent: MediaDevices {
        }
    }

    Shortcut {
        sequence: StandardKey.New
        onActivated: onboard_dialog.showNormal()
    }
}
