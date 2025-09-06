#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

class MainWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList monitors READ monitors CONSTANT)

public:
    explicit MainWindow(QObject *parent = nullptr);

    QVariantList monitors() const;

    Q_INVOKABLE void apply();
    Q_INVOKABLE void save();

private:
    QVariantList monitors_;
};