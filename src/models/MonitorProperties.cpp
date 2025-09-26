#include <algorithm>
#include <iostream>
#include <spdlog/spdlog.h>

#include "MonitorProperties.hpp"

MonitorProperties::MonitorProperties(MonitorSpecs &monitor_specs, QObject *parent)
    : QObject(parent), monitor_specs(monitor_specs) {
    spdlog::debug("Created MonitorProperties for monitor: {}", monitor_specs.getName());
}

// Getters
bool MonitorProperties::isEnabled() const { 
    return monitor_specs.isEnabled(); 
}

bool MonitorProperties::hasSettings() const {
    return monitor_specs.isEnabled() && monitor_specs.getSettings().has_value();
}

bool MonitorProperties::isAdaptiveSync() const { 
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return false;
    }
    return monitor_specs.getEnabledMonitorSettings().value().isAdaptiveSync();
}

QString MonitorProperties::getName() const { 
    return QString::fromStdString(monitor_specs.getName()); 
}

QString MonitorProperties::getDescription() const { 
    return QString::fromStdString(monitor_specs.getDescription()); 
}

float MonitorProperties::getScale() const {
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return 1.0f;
    }
    return monitor_specs.getEnabledMonitorSettings().value().getScale();
}

int MonitorProperties::getPositionX() const {
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return 0;
    }
    return monitor_specs.getEnabledMonitorSettings().value().getPosition().x;
}

int MonitorProperties::getPositionY() const {
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return 0;
    }
    return monitor_specs.getEnabledMonitorSettings().value().getPosition().y;
}

bool MonitorProperties::isFlipped() const {
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return false;
    }
    return TransformUtils::isFlipped(monitor_specs.getEnabledMonitorSettings().value().getTransform());
}

QStringList MonitorProperties::getResolutions() const {
    QStringList resolutions;
    for (const auto &mode : monitor_specs.getModes()) {
        resolutions << QString::fromStdString(mode.toString());
    }
    return resolutions;
}

QString MonitorProperties::getTransform() const {
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return QString::fromStdString(TransformUtils::toString(Transform::NORMAL));
    }
    return QString::fromStdString(TransformUtils::toString(monitor_specs.getEnabledMonitorSettings().value().getTransform()));
}

int MonitorProperties::getActiveResolutionIndex() const {
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return 0;
    }
    return static_cast<int>(monitor_specs.getEnabledMonitorSettings().value().getActiveModeIndex());
}

QStringList MonitorProperties::getTransformList() const {
    QStringList transform_list;
    for (const auto &transform_str : TransformUtils::allStrings()) {
        transform_list << QString::fromStdString(transform_str);
    }
    return transform_list;
}

// Setters
void MonitorProperties::setEnabled(bool enabled) {
    if (monitor_specs.isEnabled() == enabled) {
        return;
    }

    monitor_specs.setEnabled(enabled);
    emit enabledChanged();
}

void MonitorProperties::setFlipped(bool flipped) {
    if (isFlipped() == flipped) {
        return;
    }

    const auto& enabled_monitor_settings = monitor_specs.getEnabledMonitorSettings().value();
    enabled_monitor_settings.setTransform(TransformUtils::getFlipped(enabled_monitor_settings.getTransform()));
    emit flippedChanged();
}

void MonitorProperties::setAdaptiveSync(bool adaptiveSync) {
    if (isAdaptiveSync() == adaptiveSync) {
        return;
    }

    monitor_specs.getEnabledMonitorSettings().value().setAdaptiveSync(adaptiveSync);
    emit adaptiveSyncChanged();
}

void MonitorProperties::setScale(float scale) {
    if (qFuzzyCompare(getScale(), scale)) {
        return;
    }

    monitor_specs.getEnabledMonitorSettings().value().setScale(scale);
    emit scaleChanged();
}

void MonitorProperties::setPositionX(int x) {
    if (getPositionX() == x) {
        return;
    }

    monitor_specs.getEnabledMonitorSettings().value().setPositionX(x);
    emit positionXChanged();
}

void MonitorProperties::setPositionY(int y) {
    if (getPositionY() == y) {
        return;
    }

    monitor_specs.getEnabledMonitorSettings().value().setPositionY(y);
    emit positionYChanged();
}

void MonitorProperties::setActiveResolutionIndex(int index) {
    if (getActiveResolutionIndex() == index) {
        return;
    }

    monitor_specs.getEnabledMonitorSettings().value().setActiveModeIndex(static_cast<size_t>(index));
    emit activeResolutionIndexChanged();
}

void MonitorProperties::setTransform(const QString &transform) {
    if (!hasSettings()) {
        return;
    }
    
    Transform currentTransform = monitor_specs.getSettings().value().getTransform();
    Transform newTransform = TransformUtils::fromString(transform.toStdString());
    
    if (currentTransform == newTransform) {
        return;
    }

    monitor_specs.setTransform(newTransform);
    emit transformChanged();
}
