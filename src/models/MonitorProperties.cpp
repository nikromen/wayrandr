#include <algorithm>

#include "MonitorProperties.hpp"

MonitorProperties::MonitorProperties(MonitorSpecs &monitor_specs, QObject *parent)
    : monitor_specs(monitor_specs), QObject(parent) {}

// Getters
bool MonitorProperties::isEnabled() const { return monitor_specs.isEnabled(); }
bool MonitorProperties::isAdaptiveSync() const { return monitor_specs.isAdaptiveSync(); }
QString MonitorProperties::getName() const { return QString::fromStdString(monitor_specs.getName()); }
QString MonitorProperties::getDescription() const { return QString::fromStdString(monitor_specs.getDescription()); }
float MonitorProperties::getScale() const { return monitor_specs.getScale(); }

int MonitorProperties::getPositionX() const {
    return monitor_specs.getPosition()->x;
}

int MonitorProperties::getPositionY() const {
    return monitor_specs.getPosition()->y;
}

bool MonitorProperties::isFlipped() const {
    return TransformUtils::isFlipped(monitor_specs.getTransform());
}

QStringList MonitorProperties::getResolutions() const {
    QStringList resolutions;
    for (const auto &mode : monitor_specs.getModes()) {
        resolutions << QString::fromStdString(mode.toString());
    }
    return resolutions;
}

QString MonitorProperties::getTransform() const {
    return QString::fromStdString(TransformUtils::toString(monitor_specs.getTransform()));
}

int MonitorProperties::getActiveResolutionIndex() const {
    return &monitor_specs.getActiveMode() - monitor_specs.getModes().data();
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

    monitor_specs.setTransform(TransformUtils::getFlipped(monitor_specs.getTransform()));
    emit flippedChanged();
}

void MonitorProperties::setAdaptiveSync(bool adaptiveSync) {
    if (monitor_specs.isAdaptiveSync() == adaptiveSync) {
        return;
    }

    monitor_specs.setAdaptiveSync(adaptiveSync);
    emit adaptiveSyncChanged();
}

void MonitorProperties::setScale(float scale) {
    if (monitor_specs.getScale() == scale) {
        return;
    }

    monitor_specs.setScale(scale);
    emit scaleChanged();
}

void MonitorProperties::setPositionX(int x) {
    if (monitor_specs.getPosition()->x == x) {
        return;
    }

    monitor_specs.setPositionX(x);
    emit positionXChanged();
}

void MonitorProperties::setPositionY(int y) {
    if (monitor_specs.getPosition()->y == y) {
        return;
    }

    monitor_specs.setPositionY(y);
    emit positionYChanged();
}

void MonitorProperties::setActiveResolutionIndex(int index) {
    if (getActiveResolutionIndex() == index) {
        return;
    }

    monitor_specs.setActiveModeIndex(index);
    emit activeResolutionIndexChanged();
}

void MonitorProperties::setTransform(const QString &transform) {
    if (getTransform() == transform) {
        return;
    }

    Transform transform_converted = TransformUtils::fromString(transform.toStdString());
    monitor_specs.setTransform(transform_converted);
    emit transformChanged();
}
