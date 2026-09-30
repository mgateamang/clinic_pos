#include "pos_controller.hpp"
#include <QSettings>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

namespace aidans {

PosController::PosController(const QString& configPath, QObject* parent)
    : QObject(parent), m_configPath(configPath) {
    loadConfig();
}

void PosController::loadConfig() {
    if (QFileInfo::exists(m_configPath)) {
        QSettings settings(m_configPath, QSettings::IniFormat);
        settings.beginGroup("Server");
        QString url = settings.value("url", m_serverUrl).toString();
        m_redisHost = settings.value("redis_host", m_redisHost).toString();
        m_redisPort = settings.value("redis_port", m_redisPort).toInt();
        settings.endGroup();
        if (!url.isEmpty()) {
            m_serverUrl = url;
            emit serverUrlChanged();
        }
        emit redisConfigChanged();
    }
    initRedisSubscriber();
}

void PosController::saveConfig(const QString& url, const QString& redisHost, int redisPort) {
    QString trimmedUrl = url.trimmed();
    QString trimmedRedisHost = redisHost.trimmed().isEmpty() ? "localhost" : redisHost.trimmed();
    int validPort = redisPort <= 0 ? 6379 : redisPort;

    if (!trimmedUrl.isEmpty()) {
        m_serverUrl = trimmedUrl;
        emit serverUrlChanged();
    }

    m_redisHost = trimmedRedisHost;
    m_redisPort = validPort;
    emit redisConfigChanged();

    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.beginGroup("Server");
    settings.setValue("url", m_serverUrl);
    settings.setValue("redis_host", m_redisHost);
    settings.setValue("redis_port", m_redisPort);
    settings.endGroup();
    settings.sync();

    reconnectRedis();
    setStatus("Configuration saved to " + m_configPath);
}

void PosController::saveServerUrl(const QString& url) {
    saveConfig(url, m_redisHost, m_redisPort);
}

void PosController::initRedisSubscriber() {
    if (!m_redisSocket) {
        m_redisSocket = new QTcpSocket(this);

        connect(m_redisSocket, &QTcpSocket::connected, this, [this]() {
            m_isRedisConnected = true;
            m_redisStatus = QString("Connected to %1:%2 (listening on inventory_updates)").arg(m_redisHost).arg(m_redisPort);
            emit isRedisConnectedChanged();
            emit redisStatusChanged();

            // Send Redis SUBSCRIBE command for inventory updates channel
            m_redisSocket->write("SUBSCRIBE inventory_updates\r\n");
            m_redisSocket->flush();
            setStatus("Connected to Redis real-time sync.");
        });

        connect(m_redisSocket, &QTcpSocket::disconnected, this, [this]() {
            m_isRedisConnected = false;
            m_redisStatus = "Disconnected";
            emit isRedisConnectedChanged();
            emit redisStatusChanged();
            scheduleRedisReconnect();
        });

        connect(m_redisSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
            m_isRedisConnected = false;
            m_redisStatus = "Redis offline: " + m_redisSocket->errorString();
            emit isRedisConnectedChanged();
            emit redisStatusChanged();
            scheduleRedisReconnect();
        });

        connect(m_redisSocket, &QTcpSocket::readyRead, this, [this]() {
            QByteArray data = m_redisSocket->readAll();
            // Redis pub/sub push format contains channel name and payload
            if (data.contains("inventory_updates") && (data.contains("message") || data.contains("event") || data.contains("action"))) {
                refreshData();
                setStatus("Real-time inventory ripple update received from Redis.");
            }
        });
    }

    reconnectRedis();
}

void PosController::reconnectRedis() {
    if (m_redisSocket) {
        m_redisSocket->abort();
        m_redisSocket->connectToHost(m_redisHost, m_redisPort);
    }
}

void PosController::scheduleRedisReconnect() {
    if (!m_redisReconnectTimer) {
        m_redisReconnectTimer = new QTimer(this);
        m_redisReconnectTimer->setSingleShot(true);
        connect(m_redisReconnectTimer, &QTimer::timeout, this, [this]() {
            if (!m_isRedisConnected) {
                reconnectRedis();
            }
        });
    }
    if (!m_redisReconnectTimer->isActive()) {
        m_redisReconnectTimer->start(3000);
    }
}

void PosController::setServerUrl(const QString& url) {
    m_serverUrl = url;
    emit serverUrlChanged();
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

void PosController::setServerOnline(bool online) {
    if (m_isServerOnline != online) {
        m_isServerOnline = online;
        emit isServerOnlineChanged();
    }
}

void PosController::setConnectionStatus(const QString& status) {
    if (m_connectionStatus != status) {
        m_connectionStatus = status;
        emit connectionStatusChanged();
    }
}

void PosController::testConnection(const QString& targetUrl) {
    QString urlStr = targetUrl.trimmed();
    if (urlStr.isEmpty()) urlStr = m_serverUrl;

    setBusy(true);
    setConnectionStatus("Pinging " + urlStr + "...");

    QString healthUrl = urlStr;
    if (!healthUrl.endsWith("/health") && !healthUrl.endsWith("/api/v1/health")) {
        healthUrl = urlStr + "/health";
    }

    QUrl target(healthUrl);
    QNetworkRequest req(target);
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QNetworkReply* reply = m_netManager.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, urlStr]() {
        reply->deleteLater();
        setBusy(false);

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString service = doc.object().value("service").toString();
            QString version = doc.object().value("version").toString();
            QString redisInfo = doc.object().value("redis").toString();
            setConnectionStatus(QString("Connected to %1 (%2 v%3, Redis: %4)").arg(urlStr).arg(service).arg(version).arg(redisInfo));
            setServerOnline(true);
        } else {
            setConnectionStatus("Failed to connect: " + reply->errorString());
            setServerOnline(false);
        }
    });
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
    QUrl invUrl(m_serverUrl + "/inventory");
    QNetworkRequest invReq(invUrl);
    invReq.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QNetworkReply* invReply = m_netManager.get(invReq);
    connect(invReply, &QNetworkReply::finished, this, [this, invReply]() {
        invReply->deleteLater();
        if (invReply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(invReply->readAll());
            if (doc.isArray()) {
                m_catalog = doc.toVariant().toList();
                emit catalogChanged();
                setServerOnline(true);
            }
        } else {
            setServerOnline(false);
            setStatus("Could not connect to clinic server: " + invReply->errorString());
        }
    });

    // 2. Fetch Low Stock Alerts
    QUrl alertUrl(m_serverUrl + "/inventory/alerts");
    QNetworkRequest alertReq(alertUrl);
    alertReq.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QNetworkReply* alertReply = m_netManager.get(alertReq);
    connect(alertReply, &QNetworkReply::finished, this, [this, alertReply]() {
        alertReply->deleteLater();
        setBusy(false);
        if (alertReply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(alertReply->readAll());
            if (doc.isArray()) {
                m_lowStockAlerts = doc.toVariant().toList();
                emit lowStockAlertsChanged();
                setServerOnline(true);
                setStatus("POS Catalog & Inventory Alerts updated.");
            }
        } else {
            setServerOnline(false);
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

    QUrl reqUrl(m_serverUrl + "/prescriptions/" + rxCode.trimmed());
    QNetworkRequest req(reqUrl);
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QNetworkReply* reply = m_netManager.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, rxCode]() {
        reply->deleteLater();
        setBusy(false);

        if (reply->error() == QNetworkReply::NoError) {
            setServerOnline(true);
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
            if (reply->error() >= QNetworkReply::ConnectionRefusedError && reply->error() <= QNetworkReply::UnknownNetworkError) {
                setServerOnline(false);
            }
            setStatus("Prescription not found or server error: " + reply->errorString());
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

    QUrl reqUrl(m_serverUrl + "/pos/checkout");
    QNetworkRequest req(reqUrl);
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_netManager.post(req, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        setBusy(false);

        if (reply->error() == QNetworkReply::NoError) {
            setServerOnline(true);
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
            if (reply->error() >= QNetworkReply::ConnectionRefusedError && reply->error() <= QNetworkReply::UnknownNetworkError) {
                setServerOnline(false);
            }
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

    QUrl reqUrl(m_serverUrl + "/payments/webhook");
    QNetworkRequest req(reqUrl);
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_netManager.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, txCode, status]() {
        reply->deleteLater();
        setBusy(false);
        if (reply->error() == QNetworkReply::NoError) {
            setServerOnline(true);
            setStatus(QString("Webhook simulated for %1: Status updated to %2").arg(txCode).arg(status));
        } else {
            setStatus("Webhook simulation failed.");
        }
    });
}

} // namespace aidans
