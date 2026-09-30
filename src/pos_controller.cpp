#include "pos_controller.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

namespace aidans {

PosController::PosController(QObject* parent)
    : QObject(parent) {
}

void PosController::setServerUrl(const QString& url) {
    m_serverUrl = url;
}

void PosController::setStatus(const QString& msg) {
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}

void PosController::setBusy(bool busy) {
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit isBusyChanged();
    }
}

double PosController::calculateTotal() const {
    double sum = 0.0;
    for (const auto& itemVar : m_cart) {
        QVariantMap item = itemVar.toMap();
        sum += item["subtotal"].toDouble();
    }
    return sum;
}

void PosController::updateTotal() {
    m_totalAmount = calculateTotal();
    emit totalAmountChanged();
}

bool PosController::addItemToCart(int inventoryId, const QString& name, double unitPrice, int quantity) {
    if (inventoryId <= 0 || quantity <= 0 || unitPrice < 0.0) {
        setStatus("Error: Invalid item parameters for cart.");
        return false;
    }

    // If item already in cart, increment quantity
    for (int i = 0; i < m_cart.size(); ++i) {
        QVariantMap item = m_cart[i].toMap();
        if (item["inventory_item_id"].toInt() == inventoryId) {
            int newQty = item["quantity"].toInt() + quantity;
            item["quantity"] = newQty;
            item["subtotal"] = newQty * unitPrice;
            m_cart[i] = item;
            emit cartChanged();
            updateTotal();
            setStatus(QString("Updated %1 quantity to %2").arg(name).arg(newQty));
            return true;
        }
    }

    // New cart item
    QVariantMap newItem;
    newItem["inventory_item_id"] = inventoryId;
    newItem["name"] = name;
    newItem["unit_price"] = unitPrice;
    newItem["quantity"] = quantity;
    newItem["subtotal"] = quantity * unitPrice;

    m_cart.append(newItem);
    emit cartChanged();
    updateTotal();
    setStatus(QString("Added %1 to cart").arg(name));
    return true;
}

void PosController::removeItemFromCart(int index) {
    if (index >= 0 && index < m_cart.size()) {
        m_cart.removeAt(index);
        emit cartChanged();
        updateTotal();
        setStatus("Item removed from cart");
    }
}

void PosController::clearCart() {
    if (!m_cart.isEmpty()) {
        m_cart.clear();
        emit cartChanged();
        updateTotal();
        setStatus("Cart emptied");
    }
}

QByteArray PosController::buildCheckoutPayload(const QString& paymentMethod, const QString& rxCode) const {
    QJsonObject root;
    if (!rxCode.trimmed().isEmpty()) {
        root["prescription_code"] = rxCode.trimmed();
    }
    root["payment_method"] = paymentMethod.trimmed();

    QJsonArray itemsArr;
    for (const auto& itemVar : m_cart) {
        QVariantMap itemMap = itemVar.toMap();
        QJsonObject it;
        it["inventory_item_id"] = itemMap["inventory_item_id"].toInt();
        it["quantity"] = itemMap["quantity"].toInt();
        itemsArr.append(it);
    }
    root["items"] = itemsArr;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void PosController::refreshData() {
    setBusy(true);
    setStatus("Refreshing inventory catalog and reorder alerts...");

    // 1. Fetch Inventory Catalog
    QNetworkRequest invReq(QUrl(m_serverUrl + "/inventory"));
    QNetworkReply* invReply = m_netManager.get(invReq);
    connect(invReply, &QNetworkReply::finished, this, [this, invReply]() {
        invReply->deleteLater();
        if (invReply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(invReply->readAll());
            if (doc.isArray()) {
                m_catalog = doc.toVariant().toList();
                emit catalogChanged();
            }
        }
    });

    // 2. Fetch Low Stock Alerts
    QNetworkRequest alertReq(QUrl(m_serverUrl + "/inventory/alerts"));
    QNetworkReply* alertReply = m_netManager.get(alertReq);
    connect(alertReply, &QNetworkReply::finished, this, [this, alertReply]() {
        alertReply->deleteLater();
        setBusy(false);
        if (alertReply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(alertReply->readAll());
            if (doc.isArray()) {
                m_lowStockAlerts = doc.toVariant().toList();
                emit lowStockAlertsChanged();
                setStatus("POS Catalog & Inventory Alerts updated.");
            }
        } else {
            setStatus("Could not connect to clinic server at " + m_serverUrl);
        }
    });
}

void PosController::loadPrescription(const QString& rxCode) {
    if (rxCode.trimmed().isEmpty()) {
        setStatus("Please enter a prescription code (e.g. RX-1234).");
        return;
    }

    setBusy(true);
    setStatus("Looking up prescription " + rxCode + "...");

    QNetworkRequest req(QUrl(m_serverUrl + "/prescriptions/" + rxCode.trimmed()));
    QNetworkReply* reply = m_netManager.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, rxCode]() {
        reply->deleteLater();
        setBusy(false);

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();

            QString status = obj.value("status").toString();
            if (status == "DISPENSED") {
                setStatus("Prescription " + rxCode + " has already been dispensed!");
                return;
            }

            clearCart();
            QJsonArray items = obj.value("items").toArray();
            for (const auto& itVal : items) {
                QJsonObject it = itVal.toObject();
                int invId = it.value("inventory_item_id").toInt();
                QString name = it.value("item_name").toString();
                int qty = it.value("quantity").toInt();

                // Find price from catalog if available
                double price = 10.0; // fallback default
                for (const auto& catItemVar : m_catalog) {
                    QVariantMap catMap = catItemVar.toMap();
                    if (catMap["id"].toInt() == invId) {
                        price = catMap["unit_price"].toDouble();
                        break;
                    }
                }

                addItemToCart(invId, name, price, qty);
            }

            setStatus(QString("Loaded prescription %1 (%2 items)").arg(rxCode).arg(items.size()));
        } else {
            setStatus("Prescription not found or invalid.");
        }
    });
}

void PosController::checkout(const QString& paymentMethod, const QString& rxCode) {
    if (m_cart.isEmpty()) {
        setStatus("Error: Cart is empty. Add items before checkout.");
        return;
    }

    setBusy(true);
    setStatus("Processing transaction and deducting stock...");

    QByteArray payload = buildCheckoutPayload(paymentMethod, rxCode);

    QNetworkRequest req(QUrl(m_serverUrl + "/pos/checkout"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_netManager.post(req, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        setBusy(false);

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            QString txCode = obj.value("transaction_code").toString();
            double total = obj.value("total_amount").toDouble();
            QString status = obj.value("payment_status").toString();

            m_lastReceiptCode = txCode;
            emit lastReceiptCodeChanged();
            clearCart();
            refreshData(); // Refresh catalog stock levels and alerts

            setStatus(QString("Transaction completed! Receipt #%1 (%2 EUR) [%3]").arg(txCode).arg(total, 0, 'f', 2).arg(status));
            emit checkoutCompleted(txCode, total, status);
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString errStr = doc.object().value("error").toString();
            if (errStr.isEmpty()) errStr = reply->errorString();
            setStatus("Checkout failed: " + errStr);
        }
    });
}

void PosController::simulatePaymentWebhook(const QString& txCode, const QString& status) {
    if (txCode.trimmed().isEmpty()) {
        setStatus("No transaction code to simulate webhook for.");
        return;
    }

    setBusy(true);
    setStatus("Dispatching mock webhook confirmation to clinic server...");

    QJsonObject payload;
    payload["event_id"] = "evt_sim_manual";
    payload["transaction_code"] = txCode.trimmed();
    payload["status"] = status;
    payload["gateway_reference"] = "mock_gw_ref_882";

    QNetworkRequest req(QUrl(m_serverUrl + "/payments/webhook"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_netManager.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, txCode, status]() {
        reply->deleteLater();
        setBusy(false);
        if (reply->error() == QNetworkReply::NoError) {
            setStatus(QString("Webhook simulated for %1: Status updated to %2").arg(txCode).arg(status));
        } else {
            setStatus("Webhook simulation failed.");
        }
    });
}

} // namespace aidans
