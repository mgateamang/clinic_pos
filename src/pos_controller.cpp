/**
 * @file pos_controller.cpp
 * @brief Implementation of PosController for Qt6 / QML Pharmacy POS client.
 *
 * Implements prescription code lookups, shopping cart mathematics,
 * atomic sale dispatch to /api/v1/pos/checkout, and offline state handling.
 */

#include "pos_controller.hpp" // Include pos_controller.hpp for declarations and types
#include <QSettings> // Include QSettings for declarations and types
#include <QFileInfo> // Include QFileInfo for declarations and types
#include <QJsonDocument> // Include QJsonDocument for declarations and types
#include <QJsonObject> // Include QJsonObject for declarations and types
#include <QJsonArray> // Include QJsonArray for declarations and types
#include <QUrl> // Include QUrl for declarations and types

namespace aidans { // Execute clinical workflow step

/**
 * @brief Constructs the controller and initializes config and network monitors.
 */
PosController::PosController(const QString& configPath, QObject* parent) // Execute clinical workflow step
    : QObject(parent), m_configPath(configPath) { // Execute clinical workflow step
    loadConfig(); // Execute clinical workflow step
} // Close active scope block

void PosController::loadConfig() { // Execute clinical workflow step
    if (QFileInfo::exists(m_configPath)) { // Evaluate conditional expression to branch execution flow
        QSettings settings(m_configPath, QSettings::IniFormat); // Execute clinical workflow step
        settings.beginGroup("Server"); // Execute clinical workflow step
        QString url = settings.value("url", m_serverUrl).toString(); // Assign value to target variable
        m_redisHost = settings.value("redis_host", m_redisHost).toString(); // Assign value to target variable
        m_redisPort = settings.value("redis_port", m_redisPort).toInt(); // Assign value to target variable
        settings.endGroup(); // Execute clinical workflow step
        if (!url.isEmpty()) { // Evaluate conditional expression to branch execution flow
            m_serverUrl = url; // Assign value to target variable
            emit serverUrlChanged(); // Notify QML UI bindings of underlying property state update
        } // Close active scope block
        emit redisConfigChanged(); // Notify QML UI bindings of underlying property state update
    } // Close active scope block
    initRedisSubscriber(); // Execute clinical workflow step
} // Close active scope block

void PosController::saveConfig(const QString& url, const QString& redisHost, int redisPort) { // Execute clinical workflow step
    QString trimmedUrl = url.trimmed(); // Assign value to target variable
    QString trimmedRedisHost = redisHost.trimmed().isEmpty() ? "localhost" : redisHost.trimmed(); // Assign value to target variable
    int validPort = redisPort <= 0 ? 6379 : redisPort; // Assign value to target variable

    if (!trimmedUrl.isEmpty()) { // Evaluate conditional expression to branch execution flow
        m_serverUrl = trimmedUrl; // Assign value to target variable
        emit serverUrlChanged(); // Notify QML UI bindings of underlying property state update
    } // Close active scope block

    m_redisHost = trimmedRedisHost; // Assign value to target variable
    m_redisPort = validPort; // Assign value to target variable
    emit redisConfigChanged(); // Notify QML UI bindings of underlying property state update

    QSettings settings(m_configPath, QSettings::IniFormat); // Execute clinical workflow step
    settings.beginGroup("Server"); // Execute clinical workflow step
    settings.setValue("url", m_serverUrl); // Execute clinical workflow step
    settings.setValue("redis_host", m_redisHost); // Execute clinical workflow step
    settings.setValue("redis_port", m_redisPort); // Execute clinical workflow step
    settings.endGroup(); // Execute clinical workflow step
    settings.sync(); // Execute clinical workflow step

    reconnectRedis(); // Execute clinical workflow step
    setStatus("Configuration saved to " + m_configPath); // Execute clinical workflow step
} // Close active scope block

void PosController::saveServerUrl(const QString& url) { // Execute clinical workflow step
    saveConfig(url, m_redisHost, m_redisPort); // Execute clinical workflow step
} // Close active scope block

void PosController::initRedisSubscriber() { // Execute clinical workflow step
    if (!m_redisSocket) { // Evaluate conditional expression to branch execution flow
        m_redisSocket = new QTcpSocket(this); // Assign value to target variable

        connect(m_redisSocket, &QTcpSocket::connected, this, [this]() { // Connect Qt signal to asynchronous member slot handler
            m_isRedisConnected = true; // Assign value to target variable
            m_redisStatus = QString("Connected to %1:%2 (listening on inventory_updates)").arg(m_redisHost).arg(m_redisPort); // Assign value to target variable
            emit isRedisConnectedChanged(); // Notify QML UI bindings of underlying property state update
            emit redisStatusChanged(); // Notify QML UI bindings of underlying property state update

            // Send Redis SUBSCRIBE command for inventory updates channel
            m_redisSocket->write("SUBSCRIBE inventory_updates\r\n"); // Execute clinical workflow step
            m_redisSocket->flush(); // Execute clinical workflow step
            setStatus("Connected to Redis real-time sync."); // Execute clinical workflow step
        }); // Execute clinical workflow step

        connect(m_redisSocket, &QTcpSocket::disconnected, this, [this]() { // Connect Qt signal to asynchronous member slot handler
            m_isRedisConnected = false; // Assign value to target variable
            m_redisStatus = "Disconnected"; // Assign value to target variable
            emit isRedisConnectedChanged(); // Notify QML UI bindings of underlying property state update
            emit redisStatusChanged(); // Notify QML UI bindings of underlying property state update
            scheduleRedisReconnect(); // Connect Qt signal to asynchronous member slot handler
        }); // Execute clinical workflow step

        connect(m_redisSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) { // Connect Qt signal to asynchronous member slot handler
            m_isRedisConnected = false; // Assign value to target variable
            m_redisStatus = "Redis offline: " + m_redisSocket->errorString(); // Assign value to target variable
            emit isRedisConnectedChanged(); // Notify QML UI bindings of underlying property state update
            emit redisStatusChanged(); // Notify QML UI bindings of underlying property state update
            scheduleRedisReconnect(); // Connect Qt signal to asynchronous member slot handler
        }); // Execute clinical workflow step

        connect(m_redisSocket, &QTcpSocket::readyRead, this, [this]() { // Connect Qt signal to asynchronous member slot handler
            QByteArray data = m_redisSocket->readAll(); // Assign value to target variable
            // Redis pub/sub push format contains channel name and payload
            if (data.contains("inventory_updates") && (data.contains("message") || data.contains("event") || data.contains("action"))) { // Evaluate conditional expression to branch execution flow
                refreshData(); // Execute clinical workflow step
                setStatus("Real-time inventory ripple update received from Redis."); // Execute clinical workflow step
            } // Close active scope block
        }); // Execute clinical workflow step
    } // Close active scope block

    reconnectRedis(); // Execute clinical workflow step
} // Close active scope block

void PosController::reconnectRedis() { // Execute clinical workflow step
    if (m_redisSocket) { // Evaluate conditional expression to branch execution flow
        m_redisSocket->abort(); // Execute clinical workflow step
        m_redisSocket->connectToHost(m_redisHost, m_redisPort); // Execute clinical workflow step
    } // Close active scope block
} // Close active scope block

void PosController::scheduleRedisReconnect() { // Connect Qt signal to asynchronous member slot handler
    if (!m_redisReconnectTimer) { // Evaluate conditional expression to branch execution flow
        m_redisReconnectTimer = new QTimer(this); // Assign value to target variable
        m_redisReconnectTimer->setSingleShot(true); // Execute clinical workflow step
        connect(m_redisReconnectTimer, &QTimer::timeout, this, [this]() { // Connect Qt signal to asynchronous member slot handler
            if (!m_isRedisConnected) { // Evaluate conditional expression to branch execution flow
                reconnectRedis(); // Execute clinical workflow step
            } // Close active scope block
        }); // Execute clinical workflow step
    } // Close active scope block
    if (!m_redisReconnectTimer->isActive()) { // Evaluate conditional expression to branch execution flow
        m_redisReconnectTimer->start(3000); // Execute clinical workflow step
    } // Close active scope block
} // Close active scope block

void PosController::setServerUrl(const QString& url) { // Execute clinical workflow step
    m_serverUrl = url; // Assign value to target variable
    emit serverUrlChanged(); // Notify QML UI bindings of underlying property state update
} // Close active scope block

void PosController::setStatus(const QString& msg) { // Execute clinical workflow step
    if (m_statusMessage != msg) { // Evaluate conditional expression to branch execution flow
        m_statusMessage = msg; // Assign value to target variable
        emit statusMessageChanged(); // Notify QML UI bindings of underlying property state update
    } // Close active scope block
} // Close active scope block

void PosController::setBusy(bool busy) { // Execute clinical workflow step
    if (m_isBusy != busy) { // Evaluate conditional expression to branch execution flow
        m_isBusy = busy; // Assign value to target variable
        emit isBusyChanged(); // Notify QML UI bindings of underlying property state update
    } // Close active scope block
} // Close active scope block

void PosController::setServerOnline(bool online) { // Execute clinical workflow step
    if (m_isServerOnline != online) { // Evaluate conditional expression to branch execution flow
        m_isServerOnline = online; // Assign value to target variable
        emit isServerOnlineChanged(); // Notify QML UI bindings of underlying property state update
    } // Close active scope block
} // Close active scope block

void PosController::setConnectionStatus(const QString& status) { // Execute clinical workflow step
    if (m_connectionStatus != status) { // Evaluate conditional expression to branch execution flow
        m_connectionStatus = status; // Assign value to target variable
        emit connectionStatusChanged(); // Notify QML UI bindings of underlying property state update
    } // Close active scope block
} // Close active scope block

void PosController::testConnection(const QString& targetUrl) { // Execute clinical workflow step
    QString urlStr = targetUrl.trimmed(); // Assign value to target variable
    if (urlStr.isEmpty()) urlStr = m_serverUrl; // Evaluate conditional expression to branch execution flow

    setBusy(true); // Execute clinical workflow step
    setConnectionStatus("Pinging " + urlStr + "..."); // Execute clinical workflow step

    QString healthUrl = urlStr; // Assign value to target variable
    if (!healthUrl.endsWith("/health") && !healthUrl.endsWith("/api/v1/health")) { // Evaluate conditional expression to branch execution flow
        healthUrl = urlStr + "/health"; // Assign value to target variable
    } // Close active scope block

    QUrl target(healthUrl); // Execute clinical workflow step
    QNetworkRequest req(target); // Construct HTTP request specification with target URL and headers
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Construct HTTP request specification with target URL and headers

    QNetworkReply* reply = m_netManager.get(req); // Assign value to target variable
    connect(reply, &QNetworkReply::finished, this, [this, reply, urlStr]() { // Connect Qt signal to asynchronous member slot handler
        reply->deleteLater(); // Schedule network reply object for safe asynchronous deletion
        setBusy(false); // Execute clinical workflow step

        if (reply->error() == QNetworkReply::NoError) { // Evaluate conditional expression to branch execution flow
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll()); // Read incoming response byte stream into buffer
            QString service = doc.object().value("service").toString(); // Assign value to target variable
            QString version = doc.object().value("version").toString(); // Assign value to target variable
            QString redisInfo = doc.object().value("redis").toString(); // Assign value to target variable
            setConnectionStatus(QString("Connected to %1 (%2 v%3, Redis: %4)").arg(urlStr).arg(service).arg(version).arg(redisInfo)); // Execute clinical workflow step
            setServerOnline(true); // Execute clinical workflow step
        } else { // Execute default fallback branch
            setConnectionStatus("Failed to connect: " + reply->errorString()); // Execute clinical workflow step
            setServerOnline(false); // Execute clinical workflow step
        } // Close active scope block
    }); // Execute clinical workflow step
} // Close active scope block

double PosController::calculateTotal() const { // Execute clinical workflow step
    double sum = 0.0; // Assign value to target variable
    for (const auto& itemVar : m_cart) { // Iterate through items in sequence or collection
        QVariantMap item = itemVar.toMap(); // Assign value to target variable
        sum += item["subtotal"].toDouble(); // Assign value to target variable
    } // Close active scope block
    return sum; // Return computed result to caller
} // Close active scope block

void PosController::updateTotal() { // Execute clinical workflow step
    m_totalAmount = calculateTotal(); // Assign value to target variable
    emit totalAmountChanged(); // Notify QML UI bindings of underlying property state update
} // Close active scope block

bool PosController::addItemToCart(int inventoryId, const QString& name, double unitPrice, int quantity) { // Execute clinical workflow step
    if (inventoryId <= 0 || quantity <= 0 || unitPrice < 0.0) { // Evaluate conditional expression to branch execution flow
        setStatus("Error: Invalid item parameters for cart."); // Execute clinical workflow step
        return false; // Return computed result to caller
    } // Close active scope block

    // If item already in cart, increment quantity
    for (int i = 0; i < m_cart.size(); ++i) { // Iterate through items in sequence or collection
        QVariantMap item = m_cart[i].toMap(); // Assign value to target variable
        if (item["inventory_item_id"].toInt() == inventoryId) { // Evaluate conditional expression to branch execution flow
            int newQty = item["quantity"].toInt() + quantity; // Assign value to target variable
            item["quantity"] = newQty; // Assign value to target variable
            item["subtotal"] = newQty * unitPrice; // Assign value to target variable
            m_cart[i] = item; // Assign value to target variable
            emit cartChanged(); // Notify QML UI bindings of underlying property state update
            updateTotal(); // Execute clinical workflow step
            setStatus(QString("Updated %1 quantity to %2").arg(name).arg(newQty)); // Execute clinical workflow step
            return true; // Return computed result to caller
        } // Close active scope block
    } // Close active scope block

    // New cart item
    QVariantMap newItem; // Execute clinical workflow step
    newItem["inventory_item_id"] = inventoryId; // Assign value to target variable
    newItem["name"] = name; // Assign value to target variable
    newItem["unit_price"] = unitPrice; // Assign value to target variable
    newItem["quantity"] = quantity; // Assign value to target variable
    newItem["subtotal"] = quantity * unitPrice; // Assign value to target variable

    m_cart.append(newItem); // Execute clinical workflow step
    emit cartChanged(); // Notify QML UI bindings of underlying property state update
    updateTotal(); // Execute clinical workflow step
    setStatus(QString("Added %1 to cart").arg(name)); // Execute clinical workflow step
    return true; // Return computed result to caller
} // Close active scope block

void PosController::removeItemFromCart(int index) { // Execute clinical workflow step
    if (index >= 0 && index < m_cart.size()) { // Evaluate conditional expression to branch execution flow
        m_cart.removeAt(index); // Execute clinical workflow step
        emit cartChanged(); // Notify QML UI bindings of underlying property state update
        updateTotal(); // Execute clinical workflow step
        setStatus("Item removed from cart"); // Execute clinical workflow step
    } // Close active scope block
} // Close active scope block

void PosController::clearCart() { // Execute clinical workflow step
    if (!m_cart.isEmpty()) { // Evaluate conditional expression to branch execution flow
        m_cart.clear(); // Execute clinical workflow step
        emit cartChanged(); // Notify QML UI bindings of underlying property state update
        updateTotal(); // Execute clinical workflow step
        setStatus("Cart emptied"); // Execute clinical workflow step
    } // Close active scope block
} // Close active scope block

QByteArray PosController::buildCheckoutPayload(const QString& paymentMethod, const QString& rxCode) const { // Execute clinical workflow step
    QJsonObject root; // Execute clinical workflow step
    if (!rxCode.trimmed().isEmpty()) { // Evaluate conditional expression to branch execution flow
        root["prescription_code"] = rxCode.trimmed(); // Assign value to target variable
    } // Close active scope block
    root["payment_method"] = paymentMethod.trimmed(); // Assign value to target variable

    QJsonArray itemsArr; // Execute clinical workflow step
    for (const auto& itemVar : m_cart) { // Iterate through items in sequence or collection
        QVariantMap itemMap = itemVar.toMap(); // Assign value to target variable
        QJsonObject it; // Execute clinical workflow step
        it["inventory_item_id"] = itemMap["inventory_item_id"].toInt(); // Assign value to target variable
        it["quantity"] = itemMap["quantity"].toInt(); // Assign value to target variable
        itemsArr.append(it); // Execute clinical workflow step
    } // Close active scope block
    root["items"] = itemsArr; // Assign value to target variable

    return QJsonDocument(root).toJson(QJsonDocument::Compact); // Return computed result to caller
} // Close active scope block

void PosController::refreshData() { // Execute clinical workflow step
    setBusy(true); // Execute clinical workflow step
    setStatus("Refreshing inventory catalog and reorder alerts..."); // Execute clinical workflow step

    // 1. Fetch Inventory Catalog
    QUrl invUrl(m_serverUrl + "/inventory"); // Execute clinical workflow step
    QNetworkRequest invReq(invUrl); // Construct HTTP request specification with target URL and headers
    invReq.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Construct HTTP request specification with target URL and headers

    QNetworkReply* invReply = m_netManager.get(invReq); // Assign value to target variable
    connect(invReply, &QNetworkReply::finished, this, [this, invReply]() { // Connect Qt signal to asynchronous member slot handler
        invReply->deleteLater(); // Execute clinical workflow step
        if (invReply->error() == QNetworkReply::NoError) { // Evaluate conditional expression to branch execution flow
            QJsonDocument doc = QJsonDocument::fromJson(invReply->readAll()); // Assign value to target variable
            if (doc.isArray()) { // Evaluate conditional expression to branch execution flow
                m_catalog = doc.toVariant().toList(); // Assign value to target variable
                emit catalogChanged(); // Notify QML UI bindings of underlying property state update
                setServerOnline(true); // Execute clinical workflow step
            } // Close active scope block
        } else { // Execute default fallback branch
            setServerOnline(false); // Execute clinical workflow step
            setStatus("Could not connect to clinic server: " + invReply->errorString()); // Execute clinical workflow step
        } // Close active scope block
    }); // Execute clinical workflow step

    // 2. Fetch Low Stock Alerts
    QUrl alertUrl(m_serverUrl + "/inventory/alerts"); // Execute clinical workflow step
    QNetworkRequest alertReq(alertUrl); // Construct HTTP request specification with target URL and headers
    alertReq.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Construct HTTP request specification with target URL and headers

    QNetworkReply* alertReply = m_netManager.get(alertReq); // Assign value to target variable
    connect(alertReply, &QNetworkReply::finished, this, [this, alertReply]() { // Connect Qt signal to asynchronous member slot handler
        alertReply->deleteLater(); // Execute clinical workflow step
        setBusy(false); // Execute clinical workflow step
        if (alertReply->error() == QNetworkReply::NoError) { // Evaluate conditional expression to branch execution flow
            QJsonDocument doc = QJsonDocument::fromJson(alertReply->readAll()); // Assign value to target variable
            if (doc.isArray()) { // Evaluate conditional expression to branch execution flow
                m_lowStockAlerts = doc.toVariant().toList(); // Assign value to target variable
                emit lowStockAlertsChanged(); // Notify QML UI bindings of underlying property state update
                setServerOnline(true); // Execute clinical workflow step
                setStatus("POS Catalog & Inventory Alerts updated."); // Execute clinical workflow step
            } // Close active scope block
        } else { // Execute default fallback branch
            setServerOnline(false); // Execute clinical workflow step
            setStatus("Could not connect to clinic server at " + m_serverUrl); // Execute clinical workflow step
        } // Close active scope block
    }); // Execute clinical workflow step
} // Close active scope block

void PosController::loadPrescription(const QString& rxCode) { // Execute clinical workflow step
    if (rxCode.trimmed().isEmpty()) { // Evaluate conditional expression to branch execution flow
        setStatus("Please enter a prescription code (e.g. RX-1234)."); // Execute clinical workflow step
        return; // Exit current function execution scope
    } // Close active scope block

    setBusy(true); // Execute clinical workflow step
    setStatus("Looking up prescription " + rxCode + "..."); // Execute clinical workflow step

    QUrl reqUrl(m_serverUrl + "/prescriptions/" + rxCode.trimmed()); // Execute clinical workflow step
    QNetworkRequest req(reqUrl); // Construct HTTP request specification with target URL and headers
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Construct HTTP request specification with target URL and headers

    QNetworkReply* reply = m_netManager.get(req); // Assign value to target variable
    connect(reply, &QNetworkReply::finished, this, [this, reply, rxCode]() { // Connect Qt signal to asynchronous member slot handler
        reply->deleteLater(); // Schedule network reply object for safe asynchronous deletion
        setBusy(false); // Execute clinical workflow step

        if (reply->error() == QNetworkReply::NoError) { // Evaluate conditional expression to branch execution flow
            setServerOnline(true); // Execute clinical workflow step
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll()); // Read incoming response byte stream into buffer
            QJsonObject obj = doc.object(); // Assign value to target variable

            QString status = obj.value("status").toString(); // Assign value to target variable
            if (status == "DISPENSED") { // Evaluate conditional expression to branch execution flow
                setStatus("Prescription " + rxCode + " has already been dispensed!"); // Execute clinical workflow step
                return; // Exit current function execution scope
            } // Close active scope block

            clearCart(); // Execute clinical workflow step
            QJsonArray items = obj.value("items").toArray(); // Assign value to target variable
            for (const auto& itVal : items) { // Iterate through items in sequence or collection
                QJsonObject it = itVal.toObject(); // Assign value to target variable
                int invId = it.value("inventory_item_id").toInt(); // Assign value to target variable
                QString name = it.value("item_name").toString(); // Assign value to target variable
                int qty = it.value("quantity").toInt(); // Assign value to target variable

                // Find price from catalog if available
                double price = 10.0; // fallback default
                for (const auto& catItemVar : m_catalog) { // Iterate through items in sequence or collection
                    QVariantMap catMap = catItemVar.toMap(); // Assign value to target variable
                    if (catMap["id"].toInt() == invId) { // Evaluate conditional expression to branch execution flow
                        price = catMap["unit_price"].toDouble(); // Assign value to target variable
                        break; // Break out of current enclosing loop or switch construct
                    } // Close active scope block
                } // Close active scope block

                addItemToCart(invId, name, price, qty); // Execute clinical workflow step
            } // Close active scope block

            setStatus(QString("Loaded prescription %1 (%2 items)").arg(rxCode).arg(items.size())); // Execute clinical workflow step
        } else { // Execute default fallback branch
            if (reply->error() >= QNetworkReply::ConnectionRefusedError && reply->error() <= QNetworkReply::UnknownNetworkError) { // Evaluate conditional expression to branch execution flow
                setServerOnline(false); // Execute clinical workflow step
            } // Close active scope block
            setStatus("Prescription not found or server error: " + reply->errorString()); // Execute clinical workflow step
        } // Close active scope block
    }); // Execute clinical workflow step
} // Close active scope block

void PosController::checkout(const QString& paymentMethod, const QString& rxCode) { // Execute clinical workflow step
    if (m_cart.isEmpty()) { // Evaluate conditional expression to branch execution flow
        setStatus("Error: Cart is empty. Add items before checkout."); // Execute clinical workflow step
        return; // Exit current function execution scope
    } // Close active scope block

    setBusy(true); // Execute clinical workflow step
    setStatus("Processing transaction and deducting stock..."); // Execute clinical workflow step

    QByteArray payload = buildCheckoutPayload(paymentMethod, rxCode); // Assign value to target variable

    QUrl reqUrl(m_serverUrl + "/pos/checkout"); // Execute clinical workflow step
    QNetworkRequest req(reqUrl); // Construct HTTP request specification with target URL and headers
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Construct HTTP request specification with target URL and headers
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"); // Construct HTTP request specification with target URL and headers

    QNetworkReply* reply = m_netManager.post(req, payload); // Assign value to target variable
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { // Connect Qt signal to asynchronous member slot handler
        reply->deleteLater(); // Schedule network reply object for safe asynchronous deletion
        setBusy(false); // Execute clinical workflow step

        if (reply->error() == QNetworkReply::NoError) { // Evaluate conditional expression to branch execution flow
            setServerOnline(true); // Execute clinical workflow step
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll()); // Read incoming response byte stream into buffer
            QJsonObject obj = doc.object(); // Assign value to target variable
            QString txCode = obj.value("transaction_code").toString(); // Assign value to target variable
            double total = obj.value("total_amount").toDouble(); // Assign value to target variable
            QString status = obj.value("payment_status").toString(); // Assign value to target variable

            m_lastReceiptCode = txCode; // Assign value to target variable
            emit lastReceiptCodeChanged(); // Notify QML UI bindings of underlying property state update
            clearCart(); // Execute clinical workflow step
            refreshData(); // Refresh catalog stock levels and alerts

            setStatus(QString("Transaction completed! Receipt #%1 (%2 EUR) [%3]").arg(txCode).arg(total, 0, 'f', 2).arg(status)); // Execute clinical workflow step
            emit checkoutCompleted(txCode, total, status); // Notify QML UI bindings of underlying property state update
        } else { // Execute default fallback branch
            if (reply->error() >= QNetworkReply::ConnectionRefusedError && reply->error() <= QNetworkReply::UnknownNetworkError) { // Evaluate conditional expression to branch execution flow
                setServerOnline(false); // Execute clinical workflow step
            } // Close active scope block
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll()); // Read incoming response byte stream into buffer
            QString errStr = doc.object().value("error").toString(); // Assign value to target variable
            if (errStr.isEmpty()) errStr = reply->errorString(); // Evaluate conditional expression to branch execution flow
            setStatus("Checkout failed: " + errStr); // Execute clinical workflow step
        } // Close active scope block
    }); // Execute clinical workflow step
} // Close active scope block

void PosController::simulatePaymentWebhook(const QString& txCode, const QString& status) { // Execute clinical workflow step
    if (txCode.trimmed().isEmpty()) { // Evaluate conditional expression to branch execution flow
        setStatus("No transaction code to simulate webhook for."); // Execute clinical workflow step
        return; // Exit current function execution scope
    } // Close active scope block

    setBusy(true); // Execute clinical workflow step
    setStatus("Dispatching mock webhook confirmation to clinic server..."); // Execute clinical workflow step

    QJsonObject payload; // Execute clinical workflow step
    payload["event_id"] = "evt_sim_manual"; // Assign value to target variable
    payload["transaction_code"] = txCode.trimmed(); // Assign value to target variable
    payload["status"] = status; // Assign value to target variable
    payload["gateway_reference"] = "mock_gw_ref_882"; // Assign value to target variable

    QUrl reqUrl(m_serverUrl + "/payments/webhook"); // Execute clinical workflow step
    QNetworkRequest req(reqUrl); // Construct HTTP request specification with target URL and headers
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // Construct HTTP request specification with target URL and headers
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"); // Construct HTTP request specification with target URL and headers

    QNetworkReply* reply = m_netManager.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact)); // Assign value to target variable
    connect(reply, &QNetworkReply::finished, this, [this, reply, txCode, status]() { // Connect Qt signal to asynchronous member slot handler
        reply->deleteLater(); // Schedule network reply object for safe asynchronous deletion
        setBusy(false); // Execute clinical workflow step
        if (reply->error() == QNetworkReply::NoError) { // Evaluate conditional expression to branch execution flow
            setServerOnline(true); // Execute clinical workflow step
            setStatus(QString("Webhook simulated for %1: Status updated to %2").arg(txCode).arg(status)); // Execute clinical workflow step
        } else { // Execute default fallback branch
            setStatus("Webhook simulation failed."); // Execute clinical workflow step
        } // Close active scope block
    }); // Execute clinical workflow step
} // Close active scope block

} // namespace aidans
