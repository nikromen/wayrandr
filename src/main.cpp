#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "utils/Helpers.hpp"
#include "models/MainWindow.hpp"
#include "models/MonitorProperties.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    qmlRegisterType<MainWindow>("io.github.nikromen.wayrandr", 1, 0, "MainWindowModel");
    qmlRegisterType<MonitorProperties>("io.github.nikromen.wayrandr", 1, 0, "MonitorPropertiesModel");

    // Create the MainWindow instance on the heap to ensure it stays alive
    MainWindow* mainWindow = new MainWindow();
    
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("mainWindow", mainWindow);

    engine.load(QUrl(QStringLiteral("qrc:/qml/AppWindow.qml")));

    if (engine.rootObjects().isEmpty()) {
        delete mainWindow;
        return -1;
    }

    int result = app.exec();
    
    // Clean up
    delete mainWindow;
    
    return result;
}