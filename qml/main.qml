import Blockstream.Green 0.1
import Blockstream.Green.Core 0.1
import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Controls.Material 2.3
import QtQuick.Layouts 1.13
import QtQuick.Window 2.12

import "analytics.js" as AnalyticsJS

ApplicationWindow {
    id: window

    readonly property WalletView currentWalletView: stack_layout.currentWalletView
    readonly property Wallet currentWallet: currentWalletView ? currentWalletView.wallet : null
    readonly property Account currentAccount: currentWalletView ? currentWalletView.currentAccount : null

    property Navigation navigation: Navigation {
        location: '/home'
    }
    function link(url, text) {
        return `<style>a:link { color: "#00B45A"; text-decoration: none; }</style><a href="${url}">${text || url}</a>`
    }

    function iconFor(target) {
        if (target instanceof Wallet) return iconFor(target.network)
        if (target instanceof Network) return iconFor(target.key)
        switch (target) {
            case 'liquid':
                return 'qrc:/svg/liquid.svg'
            case 'testnet-liquid':
                return 'qrc:/svg/testnet-liquid.svg'
            case 'bitcoin':
                return 'qrc:/svg/btc.svg'
            case 'testnet':
                return 'qrc:/svg/btc_testnet.svg'
        }
        return ''
    }

    property Constants constants: Constants {}

    function formatTransactionTimestamp(tx) {
        return new Date(tx.created_at_ts / 1000).toLocaleString(locale.dateTimeFormat(Locale.LongFormat))
    }

    function accountName(account) {
        if (!account) return ''
        if (account.name !== '') return account.name
        if (account.mainAccount) return qsTrId('id_main_account')
        return qsTrId('Account %1').arg(account.pointer)
    }

    function renameAccount(account, text, active_focus) {
        if (account.rename(text, active_focus)) {
            Analytics.recordEvent('account_rename', AnalyticsJS.segmentationSubAccount(account))
        }
    }

    function dynamicScenePosition(item, x, y) {
        const target = item
        while (item) {
            item.x
            item.y
            item = item.parent
        }
        return target.mapToItem(null, x, y)
    }

    x: Settings.windowX
    y: Settings.windowY
    width: Settings.windowWidth
    height: Settings.windowHeight

    onXChanged: Settings.windowX = x
    onYChanged: Settings.windowY = y
    onWidthChanged: Settings.windowWidth = width
    onHeightChanged: Settings.windowHeight = height
    onCurrentWalletChanged: {
        if (currentWallet && currentWallet.persisted) {
            Settings.updateRecentWallet(currentWallet.id)
        }
    }

    minimumWidth: 900
    minimumHeight: 600
    visible: true
    color: constants.c900
    title: {
        const parts = Qt.application.arguments.indexOf('--debugnavigation') > 0 ? [navigation.location] : []
        if (currentWallet) {
            parts.push(font_metrics.elidedText(currentWallet.name, Qt.ElideRight, window.width / 3));
            if (currentAccount) parts.push(font_metrics.elidedText(accountName(currentAccount), Qt.ElideRight, window.width / 3));
        }
        parts.push('Blockstream Green');
        if (build_type !== 'release') parts.push(`[${build_type}]`)
        return parts.join(' - ');
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
        ColumnLayout {
            StackLayout {
                id: stack_layout
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property WalletView currentWalletView: currentIndex < 0 ? null : (stack_layout.children[currentIndex].currentWalletView || null)
                Binding on currentIndex {
                    delayed: true
                    value: {
                        let index = stack_layout.currentIndex
                        for (let i = 0; i < stack_layout.children.length; ++i) {
                            const child = stack_layout.children[i]
                            if (!(child instanceof Item)) continue
                            if (child.active && child.enabled) index = i
                        }
                        return index
                    }
                }
                HomeView {
                    readonly property bool active: navigation.path === '/home'
                }
                BlockstreamView {
                    id: blockstream_view
                    readonly property bool active: navigation.path === '/blockstream'
                }
                PreferencesView {
                    readonly property bool active: navigation.path === '/preferences'
                }
                JadeView {
                    id: jade_view
                    readonly property bool active: navigation.path.startsWith('/jade')
                }
                LedgerDevicesView {
                    id: ledger_view
                    readonly property bool active: navigation.path.startsWith('/ledger')
                }
                NetworkView {
                    network: 'bitcoin'
                    title: qsTrId('id_bitcoin_wallets')
                }
                NetworkView {
                    network: 'liquid'
                    title: qsTrId('id_liquid_wallets')
                }
                NetworkView {
                    enabled: Settings.enableTestnet
                    network: 'testnet-liquid'
                    title: qsTrId('id_liquid_testnet_wallets')
                }
                NetworkView {
                    enabled: Settings.enableTestnet
                    network: 'testnet'
                    title: qsTrId('id_testnet_wallets')
                }
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

    DialogLoader {
        active: navigation.path.match(/\/signup$/)
        dialog: SignupDialog {
            onRejected: navigation.pop()
        }
    }

    DialogLoader {
        active: navigation.path.match(/\/restore$/)
        dialog: RestoreDialog {
            onRejected: navigation.pop()
        }
    }

    DialogLoader {
        properties: {
            const [,, wallet_id] = navigation.path.split('/')
            const wallet = WalletManager.wallet(wallet_id)
            return { wallet }
        }
        active: properties.wallet && !properties.wallet.ready
        dialog: LoginDialog {
            onRejected: navigation.pop()
        }
    }

    DebugActiveFocus {
    }

    Component {
        id: create_account_dialog
        CreateAccountDialog {}
    }

    Component {
        id: remove_wallet_dialog
        RemoveWalletDialog {}
    }
}
