#include <QGuiApplication> // Include QGuiApplication for declarations and types
#include <QQmlApplicationEngine> // Include QQmlApplicationEngine for declarations and types
#include <QQmlContext> // Include QQmlContext for declarations and types
#include "pos_controller.hpp" // Include pos_controller.hpp for declarations and types

int main(int argc, char* argv[]) { // Execute clinical workflow step
    QGuiApplication app(argc, argv); // Execute clinical workflow step
    app.setApplicationName("AIDANS-POS"); // Execute clinical workflow step
    app.setOrganizationName("ElizabethClinic"); // Execute clinical workflow step

    QString configPath = "config.ini"; // Assign value to target variable
    for (int i = 1; i < argc; ++i) { // Iterate through items in sequence or collection
        QString arg = argv[i]; // Assign value to target variable
        if (arg == "--config" && i + 1 < argc) { // Evaluate conditional expression to branch execution flow
            configPath = argv[++i]; // Assign value to target variable
        } // Close active scope block
    } // Close active scope block

    aidans::PosController controller(configPath); // Execute clinical workflow step

    for (int i = 1; i < argc; ++i) { // Iterate through items in sequence or collection
        QString arg = argv[i]; // Assign value to target variable
        if (arg == "--server" && i + 1 < argc) { // Evaluate conditional expression to branch execution flow
            controller.setServerUrl(argv[++i]); // Execute clinical workflow step
        } // Close active scope block
    } // Close active scope block

    QQmlApplicationEngine engine; // Execute clinical workflow step
    engine.rootContext()->setContextProperty("controller", &controller); // Execute clinical workflow step

    const QUrl url(QStringLiteral("qrc:/Main.qml")); // Execute clinical workflow step
    QObject::connect( // Connect Qt signal to asynchronous member slot handler
        &engine, // Execute clinical workflow step
        &QQmlApplicationEngine::objectCreated, // Execute clinical workflow step
        &app, // Execute clinical workflow step
        [url](QObject* obj, const QUrl& objUrl) { // Execute clinical workflow step
            if (!obj && url == objUrl) // Evaluate conditional expression to branch execution flow
                QCoreApplication::exit(-1); // Execute clinical workflow step
        }, // Execute clinical workflow step
        Qt::QueuedConnection // Execute clinical workflow step
    ); // Execute clinical workflow step

    engine.load(url); // Execute clinical workflow step

    return app.exec(); // Return computed result to caller
} // Close active scope block
