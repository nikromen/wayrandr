#pragma once

#include <qtmetamacros.h>

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

class MonitorProperties;

class MonitorBlock : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString monitorName READ get_monitor_name CONSTANT)
    Q_PROPERTY(QString screenImage READ get_screen_image NOTIFY screen_image_changed)

public:
    explicit MonitorBlock(
        QString monitor_name, MonitorProperties * monitor_props, QObject * parent = nullptr
    );
    ~MonitorBlock() override;

    [[nodiscard]] auto get_monitor_name() const -> QString;
    [[nodiscard]] auto get_screen_image() const -> QString;

    void start_capture();
    void stop_capture();

signals:
    void screen_image_changed();

private slots:
    void capture_frame();
    void on_process_finished(int exit_code, QProcess::ExitStatus exit_status);

private:  // NOLINT
    QString monitor_name_;
    QString screen_image_;
    QTimer * capture_timer_;
    QProcess * capture_process_;
    bool is_capturing_;
    MonitorProperties * monitor_props_;
};
