import QtQuick
import QtQuick.Controls

ListView {
    id: self
    ScrollIndicator.vertical: ScrollIndicator {
        anchors.left: self.right
        anchors.leftMargin: 4
    }
    contentWidth: self.width
    displayMarginBeginning: 300
    displayMarginEnd: 100
}
