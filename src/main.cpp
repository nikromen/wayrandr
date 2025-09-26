#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QLoggingCategory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "utils/Helpers.hpp"
#include "models/MainWindow.hpp"
#include "models/MonitorProperties.hpp"

void initializeLogger() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    auto logger = std::make_shared<spdlog::logger>("wayrandr", console_sink);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    
    spdlog::set_default_logger(logger);

#ifdef ENABLE_DEBUG_LOGS
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Debug logging enabled");
#else
    spdlog::set_level(spdlog::level::info);
#endif

    spdlog::flush_on(spdlog::level::err);
    spdlog::info("spdlog logger initialized");
}

int main(int argc, char *argv[]) {
    initializeLogger();

    QGuiApplication app(argc, argv);

    spdlog::info("Registering QML types");
    qmlRegisterType<MainWindow>("io.github.nikromen.wayrandr", 1, 0, "MainWindowModel");
    qmlRegisterType<MonitorProperties>("io.github.nikromen.wayrandr", 1, 0, "MonitorPropertiesModel");

    spdlog::info("Creating MainWindow instance");
    auto mainWindow = std::make_unique<MainWindow>();
    
    QQmlApplicationEngine engine;
    spdlog::info("Setting up QML context properties");
    engine.rootContext()->setContextProperty("mainWindow", mainWindow.get());

    const QUrl url(QStringLiteral("qrc:/qml/AppWindow.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && objUrl == url) {
                spdlog::critical("Failed to load QML UI, exiting");
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );

    spdlog::info("Loading QML UI from {}", url.toString().toStdString());
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        spdlog::critical("No root objects found after QML load");
        return -1;
    }
    
    spdlog::info("UI loaded successfully");
    spdlog::info("Starting event loop");
    return app.exec();
}