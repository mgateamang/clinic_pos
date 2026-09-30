import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 920
    height: 680
    title: "AIDANS - Pharmacy POS & Real-Time Inventory (Elizabeth's Clinic)"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Left Area: POS & Cart
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: "Pharmacy Point-of-Sale (POS)"
                    font.pixelSize: 20
                    font.bold: true
                }

                // Live Redis Sync Badge
                Rectangle {
                    height: 24
                    radius: 12
                    color: controller.isRedisConnected ? "#dcfce7" : "#f1f5f9"
                    border.color: controller.isRedisConnected ? "#86efac" : "#cbd5e1"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 6

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: controller.isRedisConnected ? "#16a34a" : "#94a3b8"
                        }

                        Label {
                            text: controller.isRedisConnected ? "Real-Time Redis Sync" : "Redis Sync Inactive"
                            font.pixelSize: 11
                            font.bold: true
                            color: controller.isRedisConnected ? "#15803d" : "#64748b"
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "⚙ Settings"
                    onClicked: {
                        serverUrlField.text = controller.serverUrl;
                        redisHostField.text = controller.redisHost;
                        redisPortField.text = controller.redisPort.toString();
                        settingsDialog.open();
                    }
                }
            }

            // Prescription Lookup Row
            GroupBox {
                title: "1. Prescription Dispense (Optional)"
                Layout.fillWidth: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 8

                    TextField {
                        id: rxCodeInput
                        placeholderText: "Enter Prescription Code (e.g. RX-...)"
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Load Rx"
                        highlighted: true
                        onClicked: controller.loadPrescription(rxCodeInput.text)
                    }
                }
            }

            // OTC Add Row
            GroupBox {
                title: "2. Over-the-Counter (OTC) Item Sale"
                Layout.fillWidth: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 8

                    ComboBox {
                        id: catCombo
                        Layout.fillWidth: true
                        model: controller.catalog
                        textRole: "name"
                    }

                    Label { text: "Qty:" }
                    SpinBox {
                        id: otcQty
                        from: 1
                        to: 50
                        value: 1
                        Layout.preferredWidth: 80
                    }

                    Button {
                        text: "+ Add to Cart"
                        onClicked: {
                            if (catCombo.currentIndex >= 0) {
                                var item = controller.catalog[catCombo.currentIndex];
                                controller.addItemToCart(item.id, item.name, item.unit_price, otcQty.value);
                                otcQty.value = 1;
                            }
                        }
                    }
                }
            }

            // Cart Table
            Label {
                text: "Current Cart Items (" + controller.cart.length + "):"
                font.bold: true
            }

            ListView {
                id: cartListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: controller.cart
                delegate: Rectangle {
                    width: cartListView.width
                    height: 40
                    color: index % 2 === 0 ? "#f8fafc" : "#ffffff"
                    border.color: "#e2e8f0"
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 12

                        Label {
                            text: modelData.name
                            font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: modelData.quantity + "x"
                            Layout.preferredWidth: 40
                        }

                        Label {
                            text: Number(modelData.unit_price).toFixed(2) + " €"
                            Layout.preferredWidth: 60
                            color: "#64748b"
                        }

                        Label {
                            text: Number(modelData.subtotal).toFixed(2) + " €"
                            font.bold: true
                            Layout.preferredWidth: 70
                        }

                        Button {
                            text: "✕"
                            onClicked: controller.removeItemFromCart(index)
                        }
                    }
                }
            }

            // Checkout Bar
            Rectangle {
                Layout.fillWidth: true
                height: 64
                color: "#f1f5f9"
                border.color: "#cbd5e1"
                radius: 6

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Label {
                        text: "Total: " + controller.totalAmount.toFixed(2) + " EUR"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#0f172a"
                    }

                    Item { Layout.fillWidth: true }

                    Label { text: "Payment:" }
                    ComboBox {
                        id: payMethodCombo
                        model: ["CASH", "CARD", "E_WALLET", "WEBHOOK_MOCK"]
                        Layout.preferredWidth: 140
                    }

                    Button {
                        text: "Clear"
                        onClicked: controller.clearCart()
                    }

                    Button {
                        text: "Checkout"
                        highlighted: true
                        enabled: !controller.isBusy && controller.cart.length > 0
                        onClicked: controller.checkout(payMethodCombo.currentText, rxCodeInput.text)
                    }
                }
            }

            // Status Bar
            Rectangle {
                Layout.fillWidth: true
                height: 32
                color: "#e2e8f0"
                radius: 4

                Label {
                    anchors.centerIn: parent
                    text: controller.statusMessage.length > 0 ? controller.statusMessage : "Ready"
                    font.pixelSize: 12
                }
            }
        }

        // Right Area: Low Stock & Reorder Alerts
        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: "#fffbeb"
            border.color: "#fde68a"
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "⚠ Reorder Alerts (" + controller.lowStockAlerts.length + ")"
                        font.bold: true
                        color: "#b45309"
                        font.pixelSize: 14
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: controller.isRedisConnected ? "#16a34a" : "#cbd5e1"
                    }
                }

                Label {
                    text: "Auto-synced via Redis Pub/Sub"
                    font.pixelSize: 11
                    color: "#78350f"
                }

                ListView {
                    id: alertList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: controller.lowStockAlerts
                    delegate: Rectangle {
                        width: alertList.width
                        height: 52
                        color: "#ffffff"
                        border.color: "#fed7aa"
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 2

                            Label {
                                text: modelData.name
                                font.bold: true
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Label {
                                    text: "Stock: " + modelData.stock_quantity
                                    font.bold: true
                                    color: "#dc2626"
                                    font.pixelSize: 11
                                }
                                Item { Layout.fillWidth: true }
                                Label {
                                    text: "Min: " + modelData.reorder_threshold
                                    font.pixelSize: 11
                                    color: "#64748b"
                                }
                            }
                        }
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "Manual Refresh"
                    onClicked: controller.refreshData()
                }

                // Webhook test tool
                GroupBox {
                    title: "Mock Webhook Settlement"
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Button {
                            Layout.fillWidth: true
                            text: "Simulate Webhook PAID"
                            enabled: controller.lastReceiptCode.length > 0
                            onClicked: controller.simulatePaymentWebhook(controller.lastReceiptCode, "PAID")
                        }
                    }
                }
            }
        }
    }

    // Settings Dialog
    Dialog {
        id: settingsDialog
        title: "Server & Real-Time Sync Configuration"
        anchors.centerIn: parent
        width: 460
        modal: true
        standardButtons: Dialog.Close

        ColumnLayout {
            width: parent.width
            spacing: 12

            Label {
                text: "Clinic Server Backend URL:"
                font.bold: true
            }

            TextField {
                id: serverUrlField
                Layout.fillWidth: true
                text: controller.serverUrl
                placeholderText: "http://localhost:8080/api/v1"
            }

            RowLayout {
                spacing: 8
                Button {
                    text: "Test Server Connection"
                    onClicked: controller.testConnection(serverUrlField.text)
                }
            }

            Label {
                text: controller.connectionStatus
                color: controller.isServerOnline ? "#15803d" : "#b91c1c"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: controller.connectionStatus.length > 0
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#e2e8f0"
            }

            Label {
                text: "Redis Real-Time Pub/Sub Broker:"
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: "Host / IP:" }
                    TextField {
                        id: redisHostField
                        Layout.fillWidth: true
                        text: controller.redisHost
                        placeholderText: "localhost"
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 100
                    Label { text: "Port:" }
                    TextField {
                        id: redisPortField
                        Layout.fillWidth: true
                        text: controller.redisPort.toString()
                        placeholderText: "6379"
                    }
                }
            }

            Label {
                text: "Status: " + controller.redisStatus
                color: controller.isRedisConnected ? "#15803d" : "#b45309"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8

                Button {
                    text: "Reconnect Redis"
                    onClicked: controller.reconnectRedis()
                }

                Button {
                    text: "Save & Apply"
                    highlighted: true
                    onClicked: {
                        controller.saveConfig(serverUrlField.text, redisHostField.text, parseInt(redisPortField.text));
                        controller.refreshData();
                        settingsDialog.close();
                    }
                }
            }
        }
    }

    // Server Offline Error Overlay
    Rectangle {
        id: offlineOverlay
        anchors.fill: parent
        color: "#f8fafc"
        visible: !controller.isServerOnline
        z: 99

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 16
            width: Math.min(480, parent.width - 40)

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 64
                height: 64
                radius: 32
                color: "#fee2e2"
                Label {
                    anchors.centerIn: parent
                    text: "⚠"
                    font.pixelSize: 32
                    color: "#dc2626"
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "Clinic Server Offline or Unreachable"
                font.pixelSize: 20
                font.bold: true
                color: "#991b1b"
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "The POS system cannot establish a connection to the backend server at:\n" + controller.serverUrl
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                color: "#64748b"
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12

                Button {
                    text: "Retry Connection"
                    highlighted: true
                    onClicked: controller.refreshData()
                }

                Button {
                    text: "⚙ Server Settings"
                    onClicked: {
                        serverUrlField.text = controller.serverUrl;
                        redisHostField.text = controller.redisHost;
                        redisPortField.text = controller.redisPort.toString();
                        settingsDialog.open();
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        controller.refreshData();
    }
}
