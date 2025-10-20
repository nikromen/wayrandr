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
    return monitor_specs.isEnabled() && monitor_specs.getEnabledMonitorSettings().has_value();
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
    for (const auto& mode : monitor_specs.getModes()) {
        resolutions.append(QString::fromStdString(mode.toString()));
    }
    return resolutions;
}

QStringList MonitorProperties::getTransformList() const {
    QStringList transformList;
    for (const auto& transform : TransformUtils::allStrings()) {
        transformList.append(QString::fromStdString(transform));
    }
    return transformList;
}

QString MonitorProperties::getTransform() const {
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return QString::fromStdString(TransformUtils::toString(Transform::NORMAL));
    }
    return QString::fromStdString(TransformUtils::toString(monitor_specs.getEnabledMonitorSettings().value().getTransform()));
}

size_t MonitorProperties::getActiveResolutionIndex() const {
    if (!hasSettings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs.getName());
        return 0;
    }
    return monitor_specs.getEnabledMonitorSettings().value().getActiveModeIndex();
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

    auto& settings = monitor_specs.getEnabledMonitorSettings().value();
    auto currentTransform = settings.getTransform();
    settings.setTransform(TransformUtils::getFlipped(currentTransform));
    emit flippedChanged();
}

void MonitorProperties::setAdaptiveSync(bool adaptiveSync) {
    if (isAdaptiveSync() == adaptiveSync) {
        return;
    }
    
    auto& settings = monitor_specs.getEnabledMonitorSettings().value();
    settings.setAdaptiveSync(adaptiveSync);
    emit adaptiveSyncChanged();
}

void MonitorProperties::setScale(float scale) {
    if (std::abs(getScale() - scale) < 0.001f) {
        return;
    }
    
    auto& settings = monitor_specs.getEnabledMonitorSettings().value();
    settings.setScale(scale);
    emit scaleChanged();
}

void MonitorProperties::setPositionX(int x) {
    if (getPositionX() == x) {
        return;
    }
    
    auto& settings = monitor_specs.getEnabledMonitorSettings().value();
    settings.setPositionX(x);
    emit positionXChanged();
}

void MonitorProperties::setPositionY(int y) {
    if (getPositionY() == y) {
        return;
    }

    auto& settings = monitor_specs.getEnabledMonitorSettings().value();
    settings.setPositionY(y);
    emit positionYChanged();
}

void MonitorProperties::setActiveResolutionIndex(size_t index) {
    if (getActiveResolutionIndex() == index) {
        return;
    }

    auto& settings = monitor_specs.getEnabledMonitorSettings().value();
    settings.setActiveModeIndex(index);
    emit activeResolutionIndexChanged();
}

void MonitorProperties::setTransform(const QString &transform) {
    if (!hasSettings()) {
        return;
    }
    
    auto& settings = monitor_specs.getEnabledMonitorSettings().value();
    Transform currentTransform = settings.getTransform();
    Transform newTransform = TransformUtils::fromString(transform.toStdString());
    
    if (currentTransform == newTransform) {
        return;
    }

    settings.setTransform(newTransform);
    emit transformChanged();
}