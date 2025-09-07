#include <algorithm>

#include "MonitorProperties.hpp"

MonitorProperties::MonitorProperties(MonitorSpecs &monitor_specs, QObject *parent)
    : monitor_specs(monitor_specs), QObject(parent) {}

// TODO: the defaults for optional should really give optionals not defaults, but it does not matter rn since they are disabled in qml
// some of the methods just throw error instead of handling it... fix


// Getters
bool MonitorProperties::isEnabled() const { return monitor_specs.isEnabled(); }
bool MonitorProperties::isAdaptiveSync() const { return monitor_specs.isAdaptiveSync().value_or(false); }
QString MonitorProperties::getName() const { return QString::fromStdString(monitor_specs.getName()); }
QString MonitorProperties::getDescription() const { return QString::fromStdString(monitor_specs.getDescription()); }
float MonitorProperties::getScale() const { return monitor_specs.getScale().value_or(1.0f); }

int MonitorProperties::getPositionX() const {
    if (monitor_specs.getPosition().has_value()) {
        return monitor_specs.getPosition().value().x;
    }
    return 0;
}

int MonitorProperties::getPositionY() const {
    if (monitor_specs.getPosition().has_value()) {
        return monitor_specs.getPosition().value().y;
    }
    return 0;
}

bool MonitorProperties::isFlipped() const {
    if (monitor_specs.getTransform().has_value()) {
        return TransformUtils::isFlipped(monitor_specs.getTransform().value());
    }
    return false;
}

QStringList MonitorProperties::getResolutions() const {
    QStringList resolutions;
    for (const auto &mode : monitor_specs.getModes()) {
        resolutions << QString::fromStdString(mode.toString());
    }
    return resolutions;
}

QString MonitorProperties::getTransform() const {
    if (monitor_specs.getTransform().has_value()) {
        return QString::fromStdString(TransformUtils::toString(monitor_specs.getTransform().value()));
    }
    return QString();
}

int MonitorProperties::getActiveResolutionIndex() const {
    if (monitor_specs.getActiveModeIndex().has_value()) {
        return monitor_specs.getActiveModeIndex().value();
    }
    return -1;
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

    auto &transform = monitor_specs.getTransform();
    if (transform.has_value()) {
        monitor_specs.setTransform(TransformUtils::getFlipped(transform.value()));
        emit flippedChanged();
    } else {
        throw std::runtime_error("Transform is not set");
    }
    
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
