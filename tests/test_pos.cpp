#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
#include <cassert>
#include <cmath>
#include "pos_controller.hpp"

void runPosTests() {
    std::cout << "[TEST] Running PosController Unit Tests...\n";

    // Test Config Persistence
    QString testConfig = "test_pos_config.ini";
    if (QFile::exists(testConfig)) QFile::remove(testConfig);

    {
        aidans::PosController ctrlCfg(testConfig);
        assert(ctrlCfg.serverUrl() == "http://localhost:8080/api/v1");
        ctrlCfg.saveServerUrl("http://192.168.1.99:8080/api/v1");
    }

    {
        aidans::PosController ctrlReload(testConfig);
        assert(ctrlReload.serverUrl() == "http://192.168.1.99:8080/api/v1");
    }
    if (QFile::exists(testConfig)) QFile::remove(testConfig);

    aidans::PosController ctrl;

    // 1. Initial State
    assert(ctrl.cart().isEmpty());
    assert(ctrl.totalAmount() == 0.0);
    assert(!ctrl.isBusy());
    assert(ctrl.isServerOnline());

    // 2. Reject Invalid Items
    std::cout << "[TEST] Testing validation for invalid cart additions...\n";
    bool bad1 = ctrl.addItemToCart(0, "Invalid Item", 10.0, 1); // id <= 0
    assert(!bad1);
    bool bad2 = ctrl.addItemToCart(1, "Invalid Item", 10.0, 0); // qty <= 0
    assert(!bad2);
    bool bad3 = ctrl.addItemToCart(1, "Invalid Item", -5.0, 1); // price < 0
    assert(!bad3);
    assert(ctrl.cart().isEmpty());

    // 3. Valid Additions & Math Accuracy
    std::cout << "[TEST] Testing cart additions and total price calculations...\n";
    bool ok1 = ctrl.addItemToCart(1, "Amoxicillin 500mg", 14.50, 2);
    assert(ok1);
    assert(ctrl.cart().size() == 1);
    assert(std::fabs(ctrl.totalAmount() - 29.00) < 0.001);

    bool ok2 = ctrl.addItemToCart(2, "Paracetamol 500mg", 4.20, 1);
    assert(ok2);
    assert(ctrl.cart().size() == 2);
    assert(std::fabs(ctrl.totalAmount() - 33.20) < 0.001);

    // Incrementing existing item (add 1 more Amoxicillin)
    bool ok3 = ctrl.addItemToCart(1, "Amoxicillin 500mg", 14.50, 1);
    assert(ok3);
    assert(ctrl.cart().size() == 2); // Size remains 2, qty increases to 3
    QVariantMap item0 = ctrl.cart()[0].toMap();
    assert(item0["quantity"].toInt() == 3);
    assert(std::fabs(item0["subtotal"].toDouble() - 43.50) < 0.001);
    assert(std::fabs(ctrl.totalAmount() - 47.70) < 0.001);

    // 4. OpenAPI SaleCheckoutRequest Payload Generation
    std::cout << "[TEST] Testing OpenAPI checkout payload generation...\n";
    QByteArray payload = ctrl.buildCheckoutPayload("CARD", "RX-2026-9901");
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    assert(!doc.isNull());
    assert(doc.isObject());

    QJsonObject root = doc.object();
    assert(root["payment_method"].toString() == "CARD");
    assert(root["prescription_code"].toString() == "RX-2026-9901");
    assert(root["items"].isArray());

    QJsonArray items = root["items"].toArray();
    assert(items.size() == 2);

    QJsonObject it0 = items[0].toObject();
    assert(it0["inventory_item_id"].toInt() == 1);
    assert(it0["quantity"].toInt() == 3);

    QJsonObject it1 = items[1].toObject();
    assert(it1["inventory_item_id"].toInt() == 2);
    assert(it1["quantity"].toInt() == 1);

    // 5. Item Removal
    std::cout << "[TEST] Testing item removal and total recalculation...\n";
    ctrl.removeItemFromCart(1); // Remove Paracetamol
    assert(ctrl.cart().size() == 1);
    assert(std::fabs(ctrl.totalAmount() - 43.50) < 0.001);

    // 6. Clearing Cart
    std::cout << "[TEST] Testing clearing cart...\n";
    ctrl.clearCart();
    assert(ctrl.cart().isEmpty());
    assert(ctrl.totalAmount() == 0.0);

    std::cout << "\n>>> ALL CLINIC_POS TESTS PASSED SUCCESSFULLY! <<<\n\n";
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    runPosTests();
    return 0;
}
