import QtQuick // Import module dependency for runtime execution
import QtQuick.Controls // Import module dependency for runtime execution
import QtQuick.Layouts // Import module dependency for runtime execution

ApplicationWindow { // Execute clinical workflow step
    id: root // Execute clinical workflow step
    visible: true // Execute clinical workflow step
    width: 920 // Execute clinical workflow step
    height: 680 // Execute clinical workflow step
    title: "AIDANS - Pharmacy POS & Real-Time Inventory (Elizabeth's Clinic)" // Execute clinical workflow step

    RowLayout { // Execute clinical workflow step
        anchors.fill: parent // Execute clinical workflow step
        anchors.margins: 16 // Execute clinical workflow step
        spacing: 16 // Execute clinical workflow step

        // Left Area: POS & Cart
        ColumnLayout { // Execute clinical workflow step
            Layout.fillWidth: true // Execute clinical workflow step
            Layout.fillHeight: true // Execute clinical workflow step
            spacing: 12 // Execute clinical workflow step

            RowLayout { // Execute clinical workflow step
                Layout.fillWidth: true // Execute clinical workflow step

                Label { // Execute clinical workflow step
                    text: "Pharmacy Point-of-Sale (POS)" // Execute clinical workflow step
                    font.pixelSize: 20 // Execute clinical workflow step
                    font.bold: true // Execute clinical workflow step
                } // Close active scope block

                Item { Layout.fillWidth: true } // Execute clinical workflow step

                Button { // Execute clinical workflow step
                    text: "⚙ Settings" // Execute clinical workflow step
                    onClicked: { // Execute clinical workflow step
                        serverUrlField.text = controller.serverUrl; // Assign value to target variable
                        redisHostField.text = controller.redisHost; // Assign value to target variable
                        redisPortField.text = controller.redisPort.toString(); // Assign value to target variable
                        settingsDialog.open(); // Execute clinical workflow step
                    } // Close active scope block
                } // Close active scope block
            } // Close active scope block

            // Prescription Lookup Row
            GroupBox { // Execute clinical workflow step
                title: "1. Prescription Dispense (Optional)" // Execute clinical workflow step
                Layout.fillWidth: true // Execute clinical workflow step

                RowLayout { // Execute clinical workflow step
                    anchors.fill: parent // Execute clinical workflow step
                    spacing: 8 // Execute clinical workflow step

                    TextField { // Execute clinical workflow step
                        id: rxCodeInput // Execute clinical workflow step
                        placeholderText: "Enter Prescription Code (e.g. RX-...)" // Execute clinical workflow step
                        Layout.fillWidth: true // Execute clinical workflow step
                    } // Close active scope block

                    Button { // Execute clinical workflow step
                        text: "Load Rx" // Execute clinical workflow step
                        highlighted: true // Execute clinical workflow step
                        onClicked: controller.loadPrescription(rxCodeInput.text) // Execute clinical workflow step
                    } // Close active scope block
                } // Close active scope block
            } // Close active scope block

            // OTC Add Row
            GroupBox { // Execute clinical workflow step
                title: "2. Over-the-Counter (OTC) Item Sale" // Execute clinical workflow step
                Layout.fillWidth: true // Execute clinical workflow step

                RowLayout { // Execute clinical workflow step
                    anchors.fill: parent // Execute clinical workflow step
                    spacing: 8 // Execute clinical workflow step

                    ComboBox { // Execute clinical workflow step
                        id: catCombo // Execute clinical workflow step
                        Layout.fillWidth: true // Execute clinical workflow step
                        model: controller.catalog // Execute clinical workflow step
                        textRole: "name" // Execute clinical workflow step
                    } // Close active scope block

                    Label { text: "Qty:" } // Execute clinical workflow step
                    SpinBox { // Execute clinical workflow step
                        id: otcQty // Execute clinical workflow step
                        from: 1 // Execute clinical workflow step
                        to: 50 // Execute clinical workflow step
                        value: 1 // Execute clinical workflow step
                        Layout.preferredWidth: 80 // Execute clinical workflow step
                    } // Close active scope block

                    Button { // Execute clinical workflow step
                        text: "+ Add to Cart" // Execute clinical workflow step
                        onClicked: { // Execute clinical workflow step
                            if (catCombo.currentIndex >= 0) { // Evaluate conditional expression to branch execution flow
                                var item = controller.catalog[catCombo.currentIndex]; // Assign value to target variable
                                controller.addItemToCart(item.id, item.name, item.unit_price, otcQty.value); // Execute clinical workflow step
                                otcQty.value = 1; // Assign value to target variable
                            } // Close active scope block
                        } // Close active scope block
                    } // Close active scope block
                } // Close active scope block
            } // Close active scope block

            // Cart Table
            Label { // Execute clinical workflow step
                text: "Current Cart Items (" + controller.cart.length + "):" // Execute clinical workflow step
                font.bold: true // Execute clinical workflow step
            } // Close active scope block

            ListView { // Execute clinical workflow step
                id: cartListView // Execute clinical workflow step
                Layout.fillWidth: true // Execute clinical workflow step
                Layout.fillHeight: true // Execute clinical workflow step
                clip: true // Execute clinical workflow step
                model: controller.cart // Execute clinical workflow step
                delegate: Rectangle { // Execute clinical workflow step
                    width: cartListView.width // Execute clinical workflow step
                    height: 40 // Execute clinical workflow step
                    color: index % 2 === 0 ? "#f8fafc" : "#ffffff" // Assign value to target variable
                    border.color: "#e2e8f0" // Execute clinical workflow step
                    radius: 4 // Execute clinical workflow step

                    RowLayout { // Execute clinical workflow step
                        anchors.fill: parent // Execute clinical workflow step
                        anchors.margins: 8 // Execute clinical workflow step
                        spacing: 12 // Execute clinical workflow step

                        Label { // Execute clinical workflow step
                            text: modelData.name // Execute clinical workflow step
                            font.bold: true // Execute clinical workflow step
                            Layout.fillWidth: true // Execute clinical workflow step
                            elide: Text.ElideRight // Execute clinical workflow step
                        } // Close active scope block

                        Label { // Execute clinical workflow step
                            text: modelData.quantity + "x" // Execute clinical workflow step
                            Layout.preferredWidth: 40 // Execute clinical workflow step
                        } // Close active scope block

                        Label { // Execute clinical workflow step
                            text: Number(modelData.unit_price).toFixed(2) + " €" // Execute clinical workflow step
                            Layout.preferredWidth: 60 // Execute clinical workflow step
                            color: "#64748b" // Execute clinical workflow step
                        } // Close active scope block

                        Label { // Execute clinical workflow step
                            text: Number(modelData.subtotal).toFixed(2) + " €" // Execute clinical workflow step
                            font.bold: true // Execute clinical workflow step
                            Layout.preferredWidth: 70 // Execute clinical workflow step
                        } // Close active scope block

                        Button { // Execute clinical workflow step
                            text: "✕" // Execute clinical workflow step
                            onClicked: controller.removeItemFromCart(index) // Execute clinical workflow step
                        } // Close active scope block
                    } // Close active scope block
                } // Close active scope block
            } // Close active scope block

            // Checkout Bar
            Rectangle { // Execute clinical workflow step
                Layout.fillWidth: true // Execute clinical workflow step
                height: 64 // Execute clinical workflow step
                color: "#f1f5f9" // Execute clinical workflow step
                border.color: "#cbd5e1" // Execute clinical workflow step
                radius: 6 // Execute clinical workflow step

                RowLayout { // Execute clinical workflow step
                    anchors.fill: parent // Execute clinical workflow step
                    anchors.margins: 10 // Execute clinical workflow step
                    spacing: 12 // Execute clinical workflow step

                    Label { // Execute clinical workflow step
                        text: "Total: " + controller.totalAmount.toFixed(2) + " EUR" // Execute clinical workflow step
                        font.pixelSize: 18 // Execute clinical workflow step
                        font.bold: true // Execute clinical workflow step
                        color: "#0f172a" // Execute clinical workflow step
                    } // Close active scope block

                    Item { Layout.fillWidth: true } // Execute clinical workflow step

                    Label { text: "Payment:" } // Execute clinical workflow step
                    ComboBox { // Execute clinical workflow step
                        id: payMethodCombo // Execute clinical workflow step
                        model: ["CASH", "CARD", "E_WALLET", "WEBHOOK_MOCK"] // Execute clinical workflow step
                        Layout.preferredWidth: 140 // Execute clinical workflow step
                    } // Close active scope block

                    Button { // Execute clinical workflow step
                        text: "Clear" // Execute clinical workflow step
                        onClicked: controller.clearCart() // Execute clinical workflow step
                    } // Close active scope block

                    Button { // Execute clinical workflow step
                        text: "Checkout" // Execute clinical workflow step
                        highlighted: true // Execute clinical workflow step
                        enabled: !controller.isBusy && controller.cart.length > 0 // Execute clinical workflow step
                        onClicked: controller.checkout(payMethodCombo.currentText, rxCodeInput.text) // Execute clinical workflow step
                    } // Close active scope block
                } // Close active scope block
            } // Close active scope block

            // Status Bar
            Rectangle { // Execute clinical workflow step
                Layout.fillWidth: true // Execute clinical workflow step
                height: 32 // Execute clinical workflow step
                color: "#e2e8f0" // Execute clinical workflow step
                radius: 4 // Execute clinical workflow step

                Label { // Execute clinical workflow step
                    anchors.centerIn: parent // Execute clinical workflow step
                    text: controller.statusMessage.length > 0 ? controller.statusMessage : "Ready" // Execute clinical workflow step
                    font.pixelSize: 12 // Execute clinical workflow step
                } // Close active scope block
            } // Close active scope block
        } // Close active scope block

        // Right Area: Low Stock & Reorder Alerts
        Rectangle { // Execute clinical workflow step
            Layout.preferredWidth: 280 // Execute clinical workflow step
            Layout.fillHeight: true // Execute clinical workflow step
            color: "#fffbeb" // Execute clinical workflow step
            border.color: "#fde68a" // Execute clinical workflow step
            radius: 8 // Execute clinical workflow step

            ColumnLayout { // Execute clinical workflow step
                anchors.fill: parent // Execute clinical workflow step
                anchors.margins: 12 // Execute clinical workflow step
                spacing: 8 // Execute clinical workflow step

                RowLayout { // Execute clinical workflow step
                    Layout.fillWidth: true // Execute clinical workflow step
                    Label { // Execute clinical workflow step
                        text: "⚠ Reorder Alerts (" + controller.lowStockAlerts.length + ")" // Execute clinical workflow step
                        font.bold: true // Execute clinical workflow step
                        color: "#b45309" // Execute clinical workflow step
                        font.pixelSize: 14 // Execute clinical workflow step
                    } // Close active scope block
                    Item { Layout.fillWidth: true } // Execute clinical workflow step
                    Rectangle { // Execute clinical workflow step
                        width: 8 // Execute clinical workflow step
                        height: 8 // Execute clinical workflow step
                        radius: 4 // Execute clinical workflow step
                        color: controller.isRedisConnected ? "#16a34a" : "#cbd5e1" // Execute clinical workflow step
                    } // Close active scope block
                } // Close active scope block

                Label { // Execute clinical workflow step
                    text: "Auto-synced via Redis Pub/Sub" // Execute clinical workflow step
                    font.pixelSize: 11 // Execute clinical workflow step
                    color: "#78350f" // Execute clinical workflow step
                } // Close active scope block

                ListView { // Execute clinical workflow step
                    id: alertList // Execute clinical workflow step
                    Layout.fillWidth: true // Execute clinical workflow step
                    Layout.fillHeight: true // Execute clinical workflow step
                    clip: true // Execute clinical workflow step
                    model: controller.lowStockAlerts // Execute clinical workflow step
                    delegate: Rectangle { // Execute clinical workflow step
                        width: alertList.width // Execute clinical workflow step
                        height: 52 // Execute clinical workflow step
                        color: "#ffffff" // Execute clinical workflow step
                        border.color: "#fed7aa" // Execute clinical workflow step
                        radius: 4 // Execute clinical workflow step

                        ColumnLayout { // Execute clinical workflow step
                            anchors.fill: parent // Execute clinical workflow step
                            anchors.margins: 6 // Execute clinical workflow step
                            spacing: 2 // Execute clinical workflow step

                            Label { // Execute clinical workflow step
                                text: modelData.name // Execute clinical workflow step
                                font.bold: true // Execute clinical workflow step
                                font.pixelSize: 11 // Execute clinical workflow step
                                elide: Text.ElideRight // Execute clinical workflow step
                                Layout.fillWidth: true // Execute clinical workflow step
                            } // Close active scope block

                            RowLayout { // Execute clinical workflow step
                                Label { // Execute clinical workflow step
                                    text: "Stock: " + modelData.stock_quantity // Execute clinical workflow step
                                    font.bold: true // Execute clinical workflow step
                                    color: "#dc2626" // Execute clinical workflow step
                                    font.pixelSize: 11 // Execute clinical workflow step
                                } // Close active scope block
                                Item { Layout.fillWidth: true } // Execute clinical workflow step
                                Label { // Execute clinical workflow step
                                    text: "Min: " + modelData.reorder_threshold // Execute clinical workflow step
                                    font.pixelSize: 11 // Execute clinical workflow step
                                    color: "#64748b" // Execute clinical workflow step
                                } // Close active scope block
                            } // Close active scope block
                        } // Close active scope block
                    } // Close active scope block
                } // Close active scope block

                Button { // Execute clinical workflow step
                    Layout.fillWidth: true // Execute clinical workflow step
                    text: "Manual Refresh" // Execute clinical workflow step
                    onClicked: controller.refreshData() // Execute clinical workflow step
                } // Close active scope block

            } // Close active scope block
        } // Close active scope block
    } // Close active scope block

    // Settings Dialog
    Dialog { // Execute clinical workflow step
        id: settingsDialog // Execute clinical workflow step
        title: "Server & Real-Time Sync Configuration" // Execute clinical workflow step
        anchors.centerIn: parent // Execute clinical workflow step
        width: 460 // Execute clinical workflow step
        modal: true // Execute clinical workflow step
        standardButtons: Dialog.Close // Execute clinical workflow step

        ColumnLayout { // Execute clinical workflow step
            width: parent.width // Execute clinical workflow step
            spacing: 12 // Execute clinical workflow step

            Label { // Execute clinical workflow step
                text: "Clinic Server Backend URL:" // Execute clinical workflow step
                font.bold: true // Execute clinical workflow step
            } // Close active scope block

            TextField { // Execute clinical workflow step
                id: serverUrlField // Execute clinical workflow step
                Layout.fillWidth: true // Execute clinical workflow step
                text: controller.serverUrl // Execute clinical workflow step
                placeholderText: "http://localhost:8080/api/v1"
            } // Close active scope block

            RowLayout { // Execute clinical workflow step
                spacing: 8 // Execute clinical workflow step
                Button { // Execute clinical workflow step
                    text: "Test Server Connection" // Execute clinical workflow step
                    onClicked: controller.testConnection(serverUrlField.text) // Execute clinical workflow step
                } // Close active scope block
            } // Close active scope block

            Label { // Execute clinical workflow step
                text: controller.connectionStatus // Execute clinical workflow step
                color: controller.isServerOnline ? "#15803d" : "#b91c1c" // Execute clinical workflow step
                wrapMode: Text.WordWrap // Execute clinical workflow step
                Layout.fillWidth: true // Execute clinical workflow step
                visible: controller.connectionStatus.length > 0 // Execute clinical workflow step
            } // Close active scope block

            RowLayout { // Execute clinical workflow step
                Layout.alignment: Qt.AlignRight // Execute clinical workflow step
                spacing: 8 // Execute clinical workflow step

                Button { // Execute clinical workflow step
                    text: "Save & Apply" // Execute clinical workflow step
                    highlighted: true // Execute clinical workflow step
                    onClicked: { // Execute clinical workflow step
                        controller.saveServerUrl(serverUrlField.text); // Execute clinical workflow step
                        controller.refreshData(); // Execute clinical workflow step
                        settingsDialog.close(); // Execute clinical workflow step
                    } // Close active scope block
                } // Close active scope block
            } // Close active scope block
        } // Close active scope block
    } // Close active scope block

    Component.onCompleted: { // Execute clinical workflow step
        controller.refreshData(); // Execute clinical workflow step
    } // Close active scope block
} // Close active scope block
