#include <algorithm>

#include "MonitorProperties.hpp"

MonitorProperties::MonitorProperties(MonitorSpecs &monitor_specs, QObject *parent)
    : monitor_specs(monitor_specs), QObject(parent) {}

// Getters
bool MonitorProperties::isEnabled() const { return monitor_specs.isEnabled(); }
bool MonitorProperties::isFlipped() const { return monitor_specs.isFlipped(); }
bool MonitorProperties::isAdaptiveSync() const { return monitor_specs.isAdaptiveSync(); }
QString MonitorProperties::getName() const { return QString::fromStdString(monitor_specs.getName()); }
QString MonitorProperties::getDescription() const { return QString::fromStdString(monitor_specs.getDescription()); }
int MonitorProperties::getActiveResolutionIndex() const { return monitor_specs.getActiveResolutionIndex(); }
float MonitorProperties::getScale() const { return monitor_specs.getScale(); }
int MonitorProperties::getPositionX() const { return monitor_specs.getPosition()->x; }
int MonitorProperties::getPositionY() const { return monitor_specs.getPosition()->y; }

QStringList MonitorProperties::getResolutions() const {
    QStringList resolutions;
    for (const auto &mode : monitor_specs.getModes()) {
        resolutions << QString::fromStdString(mode.toString());
    }
    return resolutions;
}

QStringList MonitorProperties::getTransform() const {
    QStringList transform;
    for (const auto &trans : monitor_specs.getTransform()) {
        transform << QString::fromStdString(TransformUtils::toString(trans));
    }
    return transform;
}
