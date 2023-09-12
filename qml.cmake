qt_add_resources(green "qtquickcontrols2"
    PREFIX "/"
    BASE qml
    FILES
        qml/qtquickcontrols2.conf
        qml/+windows/qtquickcontrols2.conf
)

SET(QML_FILES
    qml/analytics.js
    qml/util.js
    qml/SelectServerTypeView.qml
    qml/WalletSecuritySettingsView.qml
    qml/GCard.qml
    qml/TwoFactorEnableDialog.qml
    qml/MnemonicDialog.qml
    qml/LedgerDevicesView.qml
    qml/DebugRectangle.qml
    qml/WalletGeneralSettingsView.qml
    qml/NetworkView.qml
    qml/TransactionView.qml
    qml/HSpacer.qml
    qml/WalletDialog.qml
    qml/StackViewPushAction.qml
    qml/SignLiquidTransactionResolverView.qml
    qml/GHeader.qml
    qml/Wallet2faSettingsView.qml
    qml/BlockstreamView.qml
    qml/SessionBadge.qml
    qml/GComboBox.qml
    qml/TwoFactorAuthExpiryDialog.qml
    qml/Tag.qml
    qml/JadeViewDeviceNetwork.qml
    qml/AccountDelegate.qml
    qml/DeleteWalletDialog.qml
    qml/ChangePinDialog.qml
    qml/MnemonicView.qml
    qml/TwoFactorLimitDialog.qml
    qml/JadeUpdateDialog.qml
    qml/JadeSignMessageView.qml
    qml/MainMenuBar.qml
    qml/GTextField.qml
    qml/NLockTimeDialog.qml
    qml/MessageDialog.qml
    qml/SignupDialog.qml
    qml/SelectNetworkViewCard.qml
    qml/GSwitch.qml
    qml/SetUnspentOutputsStatusDialog.qml
    qml/AccountListView.qml
    qml/VSpacer.qml
    qml/WizardButtonBox.qml
    qml/SelectServerTypeViewCard.qml
    qml/AbstractDialog.qml
    qml/ControllerDialog.qml
    qml/CreateAccountDialog.qml
    qml/WizardPage.qml
    qml/Spacer.qml
    qml/WalletListDelegate.qml
    qml/AssetDelegate.qml
    qml/RemoveWalletDialog.qml
    qml/DialogFooter.qml
    qml/OutputDelegate.qml
    qml/MnemonicPage.qml
    qml/PersistentLoader.qml
    qml/AnimLoader.qml
    qml/GToolButton.qml
    qml/WalletViewFooter.qml
    qml/DiscoverServerTypeView.qml
    qml/LoginWithPasswordView.qml
    qml/PreferencesView.qml
    qml/AssetView.qml
    qml/AddressesListView.qml
    qml/LoginWithPinView.qml
    qml/AssetListView.qml
    qml/DescriptiveRadioButton.qml
    qml/SettingsPage.qml
    qml/CopyableLabel.qml
    qml/TransactionProgress.qml
    qml/BumpFeeDialog.qml
    qml/QRCode.qml
    qml/JadeSignLiquidTransactionView.qml
    qml/GButton.qml
    qml/ReceiveDialog.qml
    qml/WordField.qml
    qml/WalletViewHeader.qml
    qml/GSearchField.qml
    qml/SystemMessageDialog.qml
    qml/AmountValidator.qml
    qml/DisableAllPinsDialog.qml
    qml/JadeViewDevice.qml
    qml/MnemonicQuizPage.qml
    qml/OutputsListView.qml
    qml/LeftArrowToolTip.qml
    qml/GPane.qml
    qml/main.qml
    qml/DialogHeader.qml
    qml/WalletSettingsDialog.qml
    qml/TransactionListView.qml
    qml/CancelTwoFactorResetDialog.qml
    qml/GTextArea.qml
    qml/Collapsible.qml
    qml/ScannerPopup.qml
    qml/DiscoverServerTypeViewCard.qml
    qml/DeviceImage.qml
    qml/LoginDialog.qml
    qml/DeviceBadge.qml
    qml/SelectCoinsView.qml
    qml/AccountIdBadge.qml
    qml/AccountView.qml
    qml/TwoFactorDisableDialog.qml
    qml/TransactionDelegate.qml
    qml/AnalyticsConsentDialog.qml
    qml/BalanceItem.qml
    qml/JadeSignTransactionView.qml
    qml/SideBar.qml
    qml/AlertView.qml
    qml/WalletAdvancedSettingsView.qml
    qml/RequestTwoFactorResetDialog.qml
    qml/BlinkAnimation.qml
    qml/SendDialog.qml
    qml/FeeComboBox.qml
    qml/EditableLabel.qml
    qml/FixedErrorBadge.qml
    qml/QRCodePopup.qml
    qml/GListView.qml
    qml/ProgressIndicator.qml
    qml/ScannerView.qml
    qml/StatusBar.qml
    qml/PinView.qml
    qml/GFlickable.qml
    qml/TransactionStatusBadge.qml
    qml/PopupBalloon.qml
    qml/SectionLabel.qml
    qml/AddressDelegate.qml
    qml/Constants.qml
    qml/TransactionDoneView.qml
    qml/JadeView.qml
    qml/SideButton.qml
    qml/SettingsBox.qml
    qml/WalletRecoverySettingsView.qml
    qml/CoinDelegate.qml
    qml/MainPage.qml
    qml/WalletView.qml
    qml/SetRecoveryEmailDialog.qml
    qml/MainPageHeader.qml
    qml/MnemonicEditor.qml
    qml/LedgerSignTransactionView.qml
    qml/RestoreDialog.qml
    qml/AssetIcon.qml
    qml/SelectNetworkView.qml
    qml/WelcomePage.qml
    qml/WatchOnlyLoginDialog.qml
    qml/HomeView.qml
    qml/AuthHandlerTaskView.qml
    qml/GetCredentialsView.qml
    qml/TaskDispatcherInspector.qml
    qml/TListView.qml
    qml/GMenu.qml
    qml/GStackView.qml
    qml/OnboardView.qml
    qml/WalletDrawer.qml
    qml/ReceiveDrawer.qml
    qml/BackButton.qml
    qml/AccountAssetField.qml
    qml/CloseButton.qml
)

if (GREEN_NO_RESOURCES)
    set(QML_FILES_2)
    target_compile_definitions(green PRIVATE SOURCE_DIR=${CMAKE_SOURCE_DIR})
else()
    set(QML_FILES_2 ${QML_FILES})
endif()

qt_add_qml_module(green
    URI Blockstream.Green
    VERSION 1.0
    NO_PLUGIN
    DEPENDENCIES QtQuick
    SOURCES src/networkmanager.cpp
    QML_FILES ${QML_FILES_2}
    #ENABLE_TYPE_COMPILER
    NO_CACHEGEN
    NO_LINT
)
