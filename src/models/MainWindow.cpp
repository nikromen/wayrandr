#include "MainWindow.hpp"
#include <QVariantMap>
#include <spdlog/spdlog.h>

#include "../MonitorSpecs.hpp"
#include "MonitorProperties.hpp"

MainWindow::MainWindow(QObject *parent) : QObject(parent)
{
    spdlog::debug("Initializing MainWindow and loading monitor specifications");
    monitors_ = getMonitorSpecsList();
    spdlog::debug("Found {} monitors", monitors_.size());

    for (auto &monitor_spec : monitors_)
    {
        spdlog::debug("Creating model for monitor: {}", monitor_spec.getName());
        monitors_models_.append(new MonitorProperties(monitor_spec, this));
    }
}

QList<QObject*> MainWindow::getMonitors() const
{
    return monitors_models_;
}

void MainWindow::apply()
{
    spdlog::info("Applying monitor configuration changes");
    // Implement actual application logic here
}

void MainWindow::save()
{
    spdlog::info("Saving monitor configuration to persistent storage");
    // Implement actual saving logic here
}
