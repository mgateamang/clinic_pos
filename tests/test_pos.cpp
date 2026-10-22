#include <QCoreApplication> // Include QCoreApplication for declarations and types
#include <QFile> // Include QFile for declarations and types
#include <QJsonDocument> // Include QJsonDocument for declarations and types
#include <QJsonObject> // Include QJsonObject for declarations and types
#include <QJsonArray> // Include QJsonArray for declarations and types
#include <iostream> // Include iostream for declarations and types
#include <cassert> // Include cassert for declarations and types
#include <cmath> // Include cmath for declarations and types
#include "pos_controller.hpp" // Include pos_controller.hpp for declarations and types

void runPosTests() { // Execute clinical workflow step
    std::cout << "[TEST] Running PosController Unit Tests...\n"; // Print diagnostic informational message to standard output

    // Test Config Persistence
    QString testConfig = "test_pos_config.ini"; // Assign value to target variable
    if (QFile::exists(testConfig)) QFile::remove(testConfig); // Evaluate conditional expression to branch execution flow

    { // Scope delimiter
        aidans::PosController ctrlCfg(testConfig); // Execute clinical workflow step
        assert(ctrlCfg.serverUrl() == "http://localhost:8080/api/v1");
        assert(ctrlCfg.redisHost() == "localhost"); // Assign value to target variable
        assert(ctrlCfg.redisPort() == 6379); // Assign value to target variable
        ctrlCfg.saveConfig("http://192.168.1.99:8080/api/v1", "192.168.1.50", 6380);
    } // Close active scope block

    { // Scope delimiter
        aidans::PosController ctrlReload(testConfig); // Execute clinical workflow step
        assert(ctrlReload.serverUrl() == "http://192.168.1.99:8080/api/v1");
        assert(ctrlReload.redisHost() == "192.168.1.50"); // Assign value to target variable
        assert(ctrlReload.redisPort() == 6380); // Assign value to target variable
    } // Close active scope block
    if (QFile::exists(testConfig)) QFile::remove(testConfig); // Evaluate conditional expression to branch execution flow

    aidans::PosController ctrl; // Execute clinical workflow step

    // 1. Initial State
    assert(ctrl.cart().isEmpty()); // Execute clinical workflow step
    assert(ctrl.totalAmount() == 0.0); // Assign value to target variable
    assert(!ctrl.isBusy()); // Execute clinical workflow step
    assert(ctrl.isServerOnline()); // Execute clinical workflow step

    // 2. Reject Invalid Items
    std::cout << "[TEST] Testing validation for invalid cart additions...\n"; // Print diagnostic informational message to standard output
    bool bad1 = ctrl.addItemToCart(0, "Invalid Item", 10.0, 1); // id <= 0
    assert(!bad1); // Execute clinical workflow step
    bool bad2 = ctrl.addItemToCart(1, "Invalid Item", 10.0, 0); // qty <= 0
    assert(!bad2); // Execute clinical workflow step
    bool bad3 = ctrl.addItemToCart(1, "Invalid Item", -5.0, 1); // price < 0
    assert(!bad3); // Execute clinical workflow step
    assert(ctrl.cart().isEmpty()); // Execute clinical workflow step

    // 3. Valid Additions & Math Accuracy
    std::cout << "[TEST] Testing cart additions and total price calculations...\n"; // Print diagnostic informational message to standard output
    bool ok1 = ctrl.addItemToCart(1, "Amoxicillin 500mg", 14.50, 2); // Assign value to target variable
    assert(ok1); // Execute clinical workflow step
    assert(ctrl.cart().size() == 1); // Assign value to target variable
    assert(std::fabs(ctrl.totalAmount() - 29.00) < 0.001); // Execute clinical workflow step

    bool ok2 = ctrl.addItemToCart(2, "Paracetamol 500mg", 4.20, 1); // Assign value to target variable
    assert(ok2); // Execute clinical workflow step
    assert(ctrl.cart().size() == 2); // Assign value to target variable
    assert(std::fabs(ctrl.totalAmount() - 33.20) < 0.001); // Execute clinical workflow step

    // Incrementing existing item (add 1 more Amoxicillin)
    bool ok3 = ctrl.addItemToCart(1, "Amoxicillin 500mg", 14.50, 1); // Assign value to target variable
    assert(ok3); // Execute clinical workflow step
    assert(ctrl.cart().size() == 2); // Size remains 2, qty increases to 3
    QVariantMap item0 = ctrl.cart()[0].toMap(); // Assign value to target variable
    assert(item0["quantity"].toInt() == 3); // Assign value to target variable
    assert(std::fabs(item0["subtotal"].toDouble() - 43.50) < 0.001); // Execute clinical workflow step
    assert(std::fabs(ctrl.totalAmount() - 47.70) < 0.001); // Execute clinical workflow step

    // 4. OpenAPI SaleCheckoutRequest Payload Generation
    std::cout << "[TEST] Testing OpenAPI checkout payload generation...\n"; // Print diagnostic informational message to standard output
    QByteArray payload = ctrl.buildCheckoutPayload("CARD", "RX-2026-9901"); // Assign value to target variable
    QJsonDocument doc = QJsonDocument::fromJson(payload); // Assign value to target variable
    assert(!doc.isNull()); // Execute clinical workflow step
    assert(doc.isObject()); // Execute clinical workflow step

    QJsonObject root = doc.object(); // Assign value to target variable
    assert(root["payment_method"].toString() == "CARD"); // Assign value to target variable
    assert(root["prescription_code"].toString() == "RX-2026-9901"); // Assign value to target variable
    assert(root["items"].isArray()); // Execute clinical workflow step

    QJsonArray items = root["items"].toArray(); // Assign value to target variable
    assert(items.size() == 2); // Assign value to target variable

    QJsonObject it0 = items[0].toObject(); // Assign value to target variable
    assert(it0["inventory_item_id"].toInt() == 1); // Assign value to target variable
    assert(it0["quantity"].toInt() == 3); // Assign value to target variable

    QJsonObject it1 = items[1].toObject(); // Assign value to target variable
    assert(it1["inventory_item_id"].toInt() == 2); // Assign value to target variable
    assert(it1["quantity"].toInt() == 1); // Assign value to target variable

    // 5. Item Removal
    std::cout << "[TEST] Testing item removal and total recalculation...\n"; // Print diagnostic informational message to standard output
    ctrl.removeItemFromCart(1); // Remove Paracetamol
    assert(ctrl.cart().size() == 1); // Assign value to target variable
    assert(std::fabs(ctrl.totalAmount() - 43.50) < 0.001); // Execute clinical workflow step

    // 6. Clearing Cart
    std::cout << "[TEST] Testing clearing cart...\n"; // Print diagnostic informational message to standard output
    ctrl.clearCart(); // Execute clinical workflow step
    assert(ctrl.cart().isEmpty()); // Execute clinical workflow step
    assert(ctrl.totalAmount() == 0.0); // Assign value to target variable

    std::cout << "\n>>> ALL CLINIC_POS TESTS PASSED SUCCESSFULLY! <<<\n\n"; // Print diagnostic informational message to standard output
} // Close active scope block

int main(int argc, char* argv[]) { // Execute clinical workflow step
    QCoreApplication app(argc, argv); // Execute clinical workflow step
    runPosTests(); // Execute clinical workflow step
    return 0; // Return computed result to caller
} // Close active scope block
