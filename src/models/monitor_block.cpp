#include "models/monitor_block.hpp"

#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qoverload.h>
#include <qstringview.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <spdlog/spdlog.h>

#include <QBuffer>
#include <QImage>
#include <QTransform>
#include <stdexcept>
#include <string>
#include <utility>

#include "models/monitor_properties.hpp"
#include "monitor_specs.hpp"

MonitorBlock::MonitorBlock(
    QString monitor_name, MonitorProperties * monitor_props, QObject * parent
)
    : QObject(parent),
      monitor_name_(std::move(monitor_name)),
      capture_timer_(new QTimer(this)),
      capture_process_(nullptr),
      is_capturing_(false),
      monitor_props_(monitor_props) {
    // TODO: set later in settings
    capture_timer_->setInterval(1000 / 60);  // 15 FPS
    connect(capture_timer_, &QTimer::timeout, this, &MonitorBlock::capture_frame);

    spdlog::debug("Created MonitorBlock for: {}", monitor_name_.toStdString());
}

MonitorBlock::~MonitorBlock() {
    stop_capture();
}

auto MonitorBlock::get_monitor_name() const -> QString {
    return monitor_name_;
}

auto MonitorBlock::get_screen_image() const -> QString {
    return screen_image_;
}

void MonitorBlock::start_capture() {
    if (!capture_timer_->isActive()) {
        capture_timer_->start();
        spdlog::debug("Started capture for monitor: {}", monitor_name_.toStdString());
    }
}

void MonitorBlock::stop_capture() {
    capture_timer_->stop();

    if (capture_process_ != nullptr && capture_process_->state() != QProcess::NotRunning) {
        capture_process_->kill();
        capture_process_->waitForFinished(100);
    }

    is_capturing_ = false;
    spdlog::debug("Stopped capture for monitor: {}", monitor_name_.toStdString());
}

void MonitorBlock::capture_frame() {
    if (is_capturing_) {
        return;
    }

    // clean up old process
    if (capture_process_ != nullptr) {
        if (capture_process_->state() != QProcess::NotRunning) {
            return;  // previous capture still running
        }
        capture_process_->deleteLater();
    }

    is_capturing_ = true;
    capture_process_ = new QProcess(this);

    connect(
        capture_process_,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        &MonitorBlock::on_process_finished
    );

    QStringList const args = { "-t", "ppm", "-o", monitor_name_, "-" };
    capture_process_->start("grim", args);
}

void MonitorBlock::on_process_finished(int exit_code, QProcess::ExitStatus exit_status) {
    is_capturing_ = false;

    if (exit_status != QProcess::NormalExit || exit_code != 0) {
        if (capture_process_ != nullptr) {
            QString const error = capture_process_->readAllStandardError();
            if (!error.isEmpty()) {
                spdlog::warn(
                    "Grim capture failed for monitor {}: {}",
                    monitor_name_.toStdString(),
                    error.toStdString()
                );
            }
        }
        return;
    }

    if (capture_process_ == nullptr) {
        return;
    }

    QByteArray const image_data = capture_process_->readAllStandardOutput();

    if (image_data.isEmpty()) {
        return;
    }

    QImage image;
    if (!image.loadFromData(image_data, "PPM")) {
        spdlog::warn("Failed to load PPM data for monitor: {}", monitor_name_.toStdString());
        return;
    }

    if (monitor_props_ != nullptr && monitor_props_->has_settings()) {
        QString const transform_str = monitor_props_->get_transform();
        Transform const transform_enum = transform_utils::from_string(transform_str.toStdString());

        QTransform transform;

        if (transform_utils::is_flipped(transform_enum)) {
            transform.scale(-1, 1);
        }

        switch (transform_enum) {
            case Transform::NORMAL:
            case Transform::FLIPPED:
                // No transformation needed
                break;
            case Transform::ROTATE_90:
            case Transform::FLIPPED_90:
                transform.rotate(90);
                break;
            case Transform::ROTATE_180:
            case Transform::FLIPPED_180:
                transform.rotate(180);
                break;
            case Transform::ROTATE_270:
            case Transform::FLIPPED_270:
                transform.rotate(270);
                break;
            default:
                throw std::invalid_argument(
                    "Unknown transform: " + std::to_string(static_cast<int>(transform_enum))
                );
        }

        if (!transform.isIdentity()) {
            image = image.transformed(transform, Qt::SmoothTransformation);
        }
    }

    QByteArray byte_array;
    QBuffer buffer(&byte_array);
    buffer.open(QIODevice::WriteOnly);

    image.save(&buffer, "JPEG");

    screen_image_ = QString("data:image/jpeg;base64,%1").arg(QString(byte_array.toBase64()));

    emit screen_image_changed();
}
