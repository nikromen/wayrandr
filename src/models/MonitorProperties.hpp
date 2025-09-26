#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include "MonitorSpecs.hpp"

class MonitorProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool hasSettings READ hasSettings NOTIFY enabledChanged)
    Q_PROPERTY(QString name READ getName CONSTANT)
    Q_PROPERTY(QString description READ getDescription CONSTANT)
    Q_PROPERTY(QStringList resolutions READ getResolutions CONSTANT)  // modes
    
    // Settings properties - only valid when hasSettings is true
    Q_PROPERTY(bool flipped READ isFlipped WRITE setFlipped NOTIFY flippedChanged)
    Q_PROPERTY(bool adaptiveSync READ isAdaptiveSync WRITE setAdaptiveSync NOTIFY adaptiveSyncChanged)
    Q_PROPERTY(size_t activeResolutionIndex READ getActiveResolutionIndex WRITE setActiveResolutionIndex NOTIFY activeResolutionIndexChanged)
    Q_PROPERTY(float scale READ getScale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(int positionX READ getPositionX WRITE setPositionX NOTIFY positionXChanged)
    Q_PROPERTY(int positionY READ getPositionY WRITE setPositionY NOTIFY positionYChanged)
    Q_PROPERTY(QString transform READ getTransform WRITE setTransform NOTIFY transformChanged)
    Q_PROPERTY(QStringList transformList READ getTransformList CONSTANT)

public:
    explicit MonitorProperties(MonitorSpecs &specs, QObject *parent = nullptr);

    // Getters
    bool isEnabled() const;
    bool hasSettings() const;
    QString getName() const;
    QString getDescription() const;
    QStringList getResolutions() const;
    QStringList getTransformList() const;
    
    // Settings getters - only valid when hasSettings is true
    bool isFlipped() const;
    bool isAdaptiveSync() const;
    size_t getActiveResolutionIndex() const;
    float getScale() const;
    int getPositionX() const;
    int getPositionY() const;
    QString getTransform() const;

    // Setters
    void setEnabled(bool enabled);
    void setFlipped(bool flipped);
    void setAdaptiveSync(bool adaptiveSync);
    void setActiveResolutionIndex(size_t index);
    void setScale(float scale);
    void setPositionX(int x);
    void setPositionY(int y);
    void setTransform(const QString &transform);

signals:
    void enabledChanged();
    void flippedChanged();
    void adaptiveSyncChanged();
    void activeResolutionIndexChanged();
    void scaleChanged();
    void positionXChanged();
    void positionYChanged();
    void transformChanged();

private:
    MonitorSpecs &monitor_specs;
};
