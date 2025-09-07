#include "MainWindow.hpp"
#include <QDebug>
#include <QVariantMap>

#include "../MonitorSpecs.hpp"
#include "MonitorProperties.hpp"

MainWindow::MainWindow(QObject *parent) : QObject(parent)
{
    monitors_ = getMonitorSpecsList();

    for (auto &monitor_spec : monitors_)
    {
        monitors_models_.append(new MonitorProperties(monitor_spec, this));
    }
}

QList<QObject*> MainWindow::getMonitors() const
{
    return monitors_models_;
}

void MainWindow::apply()
{
    qDebug() << "Metoda apply() byla zavolána z QML.";
}

void MainWindow::save()
{
    qDebug() << "Metoda save() byla zavolána z QML.";
}
