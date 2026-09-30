#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "pos_controller.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("AIDANS-POS");
    app.setOrganizationName("ElizabethClinic");

    aidans::PosController controller;

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--server" && i + 1 < argc) {
            controller.setServerUrl(argv[++i]);
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controller", &controller);

    const QUrl url(QStringLiteral("qrc:/../qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );

    engine.load(url);

    return app.exec();
}
