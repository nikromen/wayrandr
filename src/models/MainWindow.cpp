#include "MainWindow.hpp"
#include <QDebug>
#include <QVariantMap>

#include "../MonitorSpecs.hpp"

MainWindow::MainWindow(QObject *parent) : QObject(parent)
{
    std::vector<MonitorSpecs> monitors_specs_list = getMonitorSpecsList();

    for (const auto &monitor_spec : monitors_specs_list)
    {
        QVariantMap model_data;
        model_data["name"] = QString::fromStdString(monitor_spec.getName());
        monitors_.append(model_data);
    }
}

QVariantList MainWindow::getMonitors() const
{
    return monitors_;
}

void MainWindow::apply()
{
    qDebug() << "Metoda apply() byla zavolána z QML.";
}

void MainWindow::save()
{
    qDebug() << "Metoda save() byla zavolána z QML.";
}
