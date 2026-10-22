#pragma once // Ensure header is included only once per translation unit

#include <QObject> // Include base Qt object class
#include <QString> // Include Qt Unicode string class
#include <QVariantList> // Include Qt variant list collection
#include <QVariantMap> // Include Qt variant map dictionary
#include <QNetworkAccessManager> // Include Qt network access coordinator
#include <QNetworkReply> // Include Qt network reply handler
#include <QTcpSocket> // Include Qt TCP socket for real-time channels
#include <QTimer> // Include Qt timer for scheduled callbacks

namespace aidans { // Enclosing clinical suite namespace

/**
 * @class PosController
 * @brief ViewModel controller for pharmacy POS checkout and prescription dispensing.
 */
class PosController : public QObject { // Inherit QObject for meta-object system
    Q_OBJECT // Macro enabling signals, slots, and properties

    Q_PROPERTY(QVariantList catalog READ catalog NOTIFY catalogChanged) // Expose catalog list to QML
    Q_PROPERTY(QVariantList lowStockAlerts READ lowStockAlerts NOTIFY lowStockAlertsChanged) // Expose low stock alerts
    Q_PROPERTY(QVariantList cart READ cart NOTIFY cartChanged) // Expose shopping cart line items to QML
    Q_PROPERTY(double totalAmount READ totalAmount NOTIFY totalAmountChanged) // Expose cart total monetary amount
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged) // Expose user status message
    Q_PROPERTY(QString lastReceiptCode READ lastReceiptCode NOTIFY lastReceiptCodeChanged) // Expose last receipt code
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged) // Expose network busy state flag
    Q_PROPERTY(bool isServerOnline READ isServerOnline NOTIFY isServerOnlineChanged) // Expose server connectivity flag
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY serverUrlChanged) // Expose active REST API server URL
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged) // Expose diagnostic text
    Q_PROPERTY(QString redisHost READ redisHost NOTIFY redisConfigChanged) // Expose cache host string
    Q_PROPERTY(int redisPort READ redisPort NOTIFY redisConfigChanged) // Expose cache port integer
    Q_PROPERTY(bool isRedisConnected READ isRedisConnected NOTIFY isRedisConnectedChanged) // Expose cache state
    Q_PROPERTY(QString redisStatus READ redisStatus NOTIFY redisStatusChanged) // Expose cache status description

public: // Public interface methods
    explicit PosController(const QString& configPath = "config.ini", QObject* parent = nullptr); // Constructor
    ~PosController() override = default; // Destructor

    QVariantList catalog() const { return m_catalog; } // Getter for catalog items
    QVariantList lowStockAlerts() const { return m_lowStockAlerts; } // Getter for low stock alerts
    QVariantList cart() const { return m_cart; } // Getter for current cart items
    double totalAmount() const { return m_totalAmount; } // Getter for cart monetary total
    QString statusMessage() const { return m_statusMessage; } // Getter for status message
    QString lastReceiptCode() const { return m_lastReceiptCode; } // Getter for generated receipt code
    bool isBusy() const { return m_isBusy; } // Getter for network busy flag
    bool isServerOnline() const { return m_isServerOnline; } // Getter for server availability flag
    QString serverUrl() const { return m_serverUrl; } // Getter for server URL
    QString connectionStatus() const { return m_connectionStatus; } // Getter for connection diagnostic text
    QString redisHost() const { return m_redisHost; } // Getter for cache host
    int redisPort() const { return m_redisPort; } // Getter for cache port
    bool isRedisConnected() const { return m_isRedisConnected; } // Getter for cache connection flag
    QString redisStatus() const { return m_redisStatus; } // Getter for cache status display text

    Q_INVOKABLE void loadConfig(); // Load INI configuration from disk
    Q_INVOKABLE void saveServerUrl(const QString& url); // Persist updated server URL
    Q_INVOKABLE void saveConfig(const QString& url, const QString& redisHost, int redisPort); // Save configuration
    Q_INVOKABLE void testConnection(const QString& targetUrl); // Test server connectivity asynchronously
    Q_INVOKABLE void reconnectRedis(); // Attempt cache reconnection
    Q_INVOKABLE void setServerUrl(const QString& url); // Set server URL dynamically
    Q_INVOKABLE void refreshData(); // Refresh catalog and stock from backend
    Q_INVOKABLE void loadPrescription(const QString& rxCode); // Load prescription by unique RX code
    Q_INVOKABLE bool addItemToCart(int inventoryId, const QString& name, double unitPrice, int quantity); // Add item to cart
    Q_INVOKABLE void removeItemFromCart(int index); // Remove item from cart by index
    Q_INVOKABLE void clearCart(); // Clear all items from current cart
    Q_INVOKABLE void checkout(const QString& paymentMethod, const QString& rxCode = ""); // Process atomic checkout
    Q_INVOKABLE void simulatePaymentWebhook(const QString& txCode, const QString& status); // Simulate webhook payment

    // Helpers
    double calculateTotal() const; // Calculate total monetary sum of items in cart
    QByteArray buildCheckoutPayload(const QString& paymentMethod, const QString& rxCode) const; // Build JSON checkout payload

signals: // Qt notification signals
    void catalogChanged(); // Emitted when catalog updates
    void lowStockAlertsChanged(); // Emitted when low stock list changes
    void cartChanged(); // Emitted when cart items update
    void totalAmountChanged(); // Emitted when total monetary amount changes
    void statusMessageChanged(); // Emitted when status message updates
    void lastReceiptCodeChanged(); // Emitted when receipt code is generated
    void isBusyChanged(); // Emitted when network busy state toggles
    void isServerOnlineChanged(); // Emitted when server availability changes
    void serverUrlChanged(); // Emitted when server URL changes
    void connectionStatusChanged(); // Emitted when connection diagnostic text updates
    void redisConfigChanged(); // Emitted when cache configuration changes
    void isRedisConnectedChanged(); // Emitted when cache connection status changes
    void redisStatusChanged(); // Emitted when cache description changes
    void checkoutCompleted(const QString& txCode, double total, const QString& status); // Emitted on successful checkout

private: // Private member state and helpers
    QString m_configPath{"config.ini"}; // Path to configuration file
    QString m_serverUrl{"http://localhost:8080/api/v1"}; // Active REST API server URL
    QString m_connectionStatus; // Detailed connection diagnostic text
    bool m_isServerOnline{true}; // Server availability status flag

    // Redis Pub/Sub Subscriber
    QString m_redisHost{"localhost"}; // Cache host string
    int m_redisPort{6379}; // Cache port number
    bool m_isRedisConnected{false}; // Cache connection flag
    QString m_redisStatus{"Disconnected"}; // Cache status display string
    QTcpSocket* m_redisSocket{nullptr}; // TCP socket for cache
    QTimer* m_redisReconnectTimer{nullptr}; // Reconnect timer for cache

    QVariantList m_catalog; // Cached list of catalog items
    QVariantList m_lowStockAlerts; // Cached list of low stock items
    QVariantList m_cart; // Active shopping cart items
    double m_totalAmount{0.0}; // Calculated cart monetary sum
    QString m_statusMessage; // Active status message text
    QString m_lastReceiptCode; // Last generated transaction receipt code
    bool m_isBusy{false}; // Busy network flag
    QNetworkAccessManager m_netManager; // Network manager instance

    void setStatus(const QString& msg); // Set status message helper
    void setBusy(bool busy); // Set busy state helper
    void setServerOnline(bool online); // Set server availability helper
    void setConnectionStatus(const QString& status); // Set diagnostic connection text helper
    void updateTotal(); // Recompute cart monetary total helper
    void initRedisSubscriber(); // Initialize cache subscriber channel
    void scheduleRedisReconnect(); // Schedule cache reconnection attempt
}; // End of PosController class definition

} // namespace aidans
