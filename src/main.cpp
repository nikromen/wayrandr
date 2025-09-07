#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "utils/Helpers.hpp"
#include "models/MainWindow.hpp"
#include "models/MonitorProperties.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    qmlRegisterType<MainWindow>("io.github.nikromen.wayrandr", 1, 0, "MainWindow");
    qmlRegisterType<MonitorProperties>("io.github.nikromen.wayrandr", 1, 0, "MonitorProperties");

    QQmlApplicationEngine engine;
    
    MainWindow mainWindow;
    engine.rootContext()->setContextProperty("mainWindow", &mainWindow);

    engine.load(QUrl(QStringLiteral("qrc:/qml/AppWindow.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}