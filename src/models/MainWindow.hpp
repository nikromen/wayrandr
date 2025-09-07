#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

#include "MonitorProperties.hpp"

class MainWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> monitors READ getMonitors CONSTANT)

public:
    explicit MainWindow(QObject *parent = nullptr);

    QList<QObject*> getMonitors() const;

    Q_INVOKABLE void apply();
    Q_INVOKABLE void save();

private:
    std::vector<MonitorSpecs> monitors_; // Store the actual monitor specs
    QList<QObject*> monitors_models_;
};