#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpSocket>
#include <QTimer>

namespace aidans {

class PosController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList catalog READ catalog NOTIFY catalogChanged)
    Q_PROPERTY(QVariantList lowStockAlerts READ lowStockAlerts NOTIFY lowStockAlertsChanged)
    Q_PROPERTY(QVariantList cart READ cart NOTIFY cartChanged)
    Q_PROPERTY(double totalAmount READ totalAmount NOTIFY totalAmountChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastReceiptCode READ lastReceiptCode NOTIFY lastReceiptCodeChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(bool isServerOnline READ isServerOnline NOTIFY isServerOnlineChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString redisHost READ redisHost NOTIFY redisConfigChanged)
    Q_PROPERTY(int redisPort READ redisPort NOTIFY redisConfigChanged)
    Q_PROPERTY(bool isRedisConnected READ isRedisConnected NOTIFY isRedisConnectedChanged)
    Q_PROPERTY(QString redisStatus READ redisStatus NOTIFY redisStatusChanged)

public:
    explicit PosController(const QString& configPath = "config.ini", QObject* parent = nullptr);
    ~PosController() override = default;

    QVariantList catalog() const { return m_catalog; }
    QVariantList lowStockAlerts() const { return m_lowStockAlerts; }
    QVariantList cart() const { return m_cart; }
    double totalAmount() const { return m_totalAmount; }
    QString statusMessage() const { return m_statusMessage; }
    QString lastReceiptCode() const { return m_lastReceiptCode; }
    bool isBusy() const { return m_isBusy; }
    bool isServerOnline() const { return m_isServerOnline; }
    QString serverUrl() const { return m_serverUrl; }
    QString connectionStatus() const { return m_connectionStatus; }
    QString redisHost() const { return m_redisHost; }
    int redisPort() const { return m_redisPort; }
    bool isRedisConnected() const { return m_isRedisConnected; }
    QString redisStatus() const { return m_redisStatus; }

    Q_INVOKABLE void loadConfig();
    Q_INVOKABLE void saveServerUrl(const QString& url);
    Q_INVOKABLE void saveConfig(const QString& url, const QString& redisHost, int redisPort);
    Q_INVOKABLE void testConnection(const QString& targetUrl);
    Q_INVOKABLE void reconnectRedis();
    Q_INVOKABLE void setServerUrl(const QString& url);
    Q_INVOKABLE void refreshData();
    Q_INVOKABLE void loadPrescription(const QString& rxCode);
    Q_INVOKABLE bool addItemToCart(int inventoryId, const QString& name, double unitPrice, int quantity);
    Q_INVOKABLE void removeItemFromCart(int index);
    Q_INVOKABLE void clearCart();
    Q_INVOKABLE void checkout(const QString& paymentMethod, const QString& rxCode = "");
    Q_INVOKABLE void simulatePaymentWebhook(const QString& txCode, const QString& status);

    // Helpers
    double calculateTotal() const;
    QByteArray buildCheckoutPayload(const QString& paymentMethod, const QString& rxCode) const;

signals:
    void catalogChanged();
    void lowStockAlertsChanged();
    void cartChanged();
    void totalAmountChanged();
    void statusMessageChanged();
    void lastReceiptCodeChanged();
    void isBusyChanged();
    void isServerOnlineChanged();
    void serverUrlChanged();
    void connectionStatusChanged();
    void redisConfigChanged();
    void isRedisConnectedChanged();
    void redisStatusChanged();
    void checkoutCompleted(const QString& txCode, double total, const QString& status);

private:
    QString m_configPath{"config.ini"};
    QString m_serverUrl{"http://localhost:8080/api/v1"};
    QString m_connectionStatus;
    bool m_isServerOnline{true};

    // Redis Pub/Sub Subscriber
    QString m_redisHost{"localhost"};
    int m_redisPort{6379};
    bool m_isRedisConnected{false};
    QString m_redisStatus{"Disconnected"};
    QTcpSocket* m_redisSocket{nullptr};
    QTimer* m_redisReconnectTimer{nullptr};

    QVariantList m_catalog;
    QVariantList m_lowStockAlerts;
    QVariantList m_cart;
    double m_totalAmount{0.0};
    QString m_statusMessage;
    QString m_lastReceiptCode;
    bool m_isBusy{false};
    QNetworkAccessManager m_netManager;

    void setStatus(const QString& msg);
    void setBusy(bool busy);
    void setServerOnline(bool online);
    void setConnectionStatus(const QString& status);
    void updateTotal();
    void initRedisSubscriber();
    void scheduleRedisReconnect();
};

} // namespace aidans
