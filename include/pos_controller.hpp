#pragma once

/**
 * @file pos_controller.hpp
 * @brief ViewModel Controller for AIDANS Pharmacy Point-of-Sale (POS) Client.
 *
 * Exposes a reactive Qt C++ interface to declarative QML UI views.
 * Responsibilities:
 *  - Orchestrates prescription lookup by RX code (`GET /api/v1/prescriptions/:code`).
 *  - Manages shopping cart state (adding line items, quantity adjustments, tax & discounts).
 *  - Dispatches atomic checkout requests (`POST /api/v1/pos/checkout`) to decrement stock
 *    and finalize sales audit receipts.
 *  - Provides live connection status monitoring and settings persistence via config.ini.
 */

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

    // Reactive properties exposed directly to QML binding expressions
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
    /**
     * @brief Constructs controller, loads INI configuration, and checks server connectivity.
     * @param configPath Path to local INI configuration file.
     * @param parent Qt parent object hierarchy pointer.
     */
    explicit PosController(const QString& configPath = "config.ini", QObject* parent = nullptr);
    ~PosController() override = default;

    // Property Getters
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

    /**
     * @brief Loads server base URL and parameters from disk config.ini.
     */
    Q_INVOKABLE void loadConfig();

    /**
     * @brief Saves server URL to config file.
     */
    Q_INVOKABLE void saveServerUrl(const QString& url);

    /**
     * @brief Saves extended network settings to config.ini.
     */
    Q_INVOKABLE void saveConfig(const QString& url, const QString& redisHost, int redisPort);

    /**
     * @brief Performs an asynchronous probe to verify server reachability.
     */
    Q_INVOKABLE void testConnection(const QString& targetUrl);

    /**
     * @brief Diagnostic redis reconnect stub (retained for backward compatibility).
     */
    Q_INVOKABLE void reconnectRedis();

    /**
     * @brief Dynamically sets active server URL.
     */
    Q_INVOKABLE void setServerUrl(const QString& url);

    /**
     * @brief Asynchronously fetches fresh medicine catalog from backend.
     */
    Q_INVOKABLE void refreshData();

    /**
     * @brief Looks up a prescription by unique code and auto-populates cart with prescribed items.
     * @param rxCode Digital prescription reference (e.g., "RX-123456-7890").
     */
    Q_INVOKABLE void loadPrescription(const QString& rxCode);

    /**
     * @brief Adds an over-the-counter or prescribed item into the active cart.
     * @param inventoryId Inventory item ID.
     * @param name Medicine display name.
     * @param unitPrice Unit sales price.
     * @param quantity Number of units purchased.
     * @return true if added successfully.
     */
    Q_INVOKABLE bool addItemToCart(int inventoryId, const QString& name, double unitPrice, int quantity);

    /**
     * @brief Removes a line item from the shopping cart.
     * @param index Zero-based item index in cart.
     */
    Q_INVOKABLE void removeItemFromCart(int index);

    /**
     * @brief Clears all items from the current cart.
     */
    Q_INVOKABLE void clearCart();

    /**
     * @brief Executes atomic sale checkout with backend server.
     * @param paymentMethod CASH, CARD, INSURANCE, or QR_TRANSFER.
     * @param cashierName Staff identifier running the checkout register.
     * @param discount Optional discount amount to deduct.
     * @param taxRate Sales tax percentage (e.g., 0.05 for 5%).
     */
    Q_INVOKABLE void checkout(const QString& paymentMethod, const QString& cashierName,
                              double discount = 0.0, double taxRate = 0.0);

    /**
     * @brief Pure calculation helper to sum cart subtotal.
     */
    static double calculateCartTotal(const QVariantList& cart);

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
    void checkoutSuccess(const QString& receiptCode, double total);
    void checkoutFailed(const QString& errorMessage);
    void prescriptionLoaded(const QString& rxCode, const QString& patientName, int itemCount);

private:
    QString m_configPath{"config.ini"};
    QString m_serverUrl{"http://localhost:8080"};
    QString m_redisHost{"localhost"};
    int m_redisPort{6379};
    bool m_isRedisConnected{false};
    QString m_redisStatus{"Disabled"};

    QVariantList m_catalog;
    QVariantList m_lowStockAlerts;
    QVariantList m_cart;
    double m_totalAmount{0.0};
    QString m_statusMessage{"Ready"};
    QString m_lastReceiptCode{""};
    QString m_activePrescriptionCode{""};
    int m_activePatientId{0};
    bool m_isBusy{false};
    bool m_isServerOnline{false};
    QString m_connectionStatus{"Checking..."};

    QNetworkAccessManager* m_netManager{nullptr};
    QTcpSocket* m_redisSocket{nullptr};
    QTimer* m_redisReconnectTimer{nullptr};

    void checkServerHealth();
    void updateCartTotal();
};

} // namespace aidans
