#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "utils/Helpers.hpp"
#include "models/MainWindow.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    
    MainWindow mainWindow;
    engine.rootContext()->setContextProperty("mainWindow", &mainWindow);

    engine.load(QUrl(QStringLiteral("qrc:/qml/AppWindow.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}