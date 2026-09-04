#include "models/monitor_block.hpp"

#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qoverload.h>
#include <qstringview.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <spdlog/spdlog.h>

#include <QProcess>
#include <utility>

namespace {
constexpr int kPreviewFps = 1;
constexpr int kPreviewIntervalMs = 1000 / kPreviewFps;
constexpr char kPreviewScale[] = "0.2";
}  // namespace

MonitorBlock::MonitorBlock(QString monitor_name, QObject * parent)
    : QObject(parent),
      monitor_name_(std::move(monitor_name)),
      capture_timer_(new QTimer(this)),
      capture_process_(nullptr),
      is_capturing_(false) {
    capture_timer_->setInterval(kPreviewIntervalMs);
    connect(capture_timer_, &QTimer::timeout, this, &MonitorBlock::capture_frame);

    spdlog::debug(
        "Created MonitorBlock for: {} (preview {} FPS)", monitor_name_.toStdString(), kPreviewFps
    );
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

void MonitorBlock::capture_now() {
    if (!capture_timer_->isActive()) {
        return;
    }

    capture_frame();
}

void MonitorBlock::capture_frame() {
    if (is_capturing_) {
        spdlog::debug(
            "Skipping capture for {} - previous grim still running", monitor_name_.toStdString()
        );
        return;
    }

    if (capture_process_ != nullptr) {
        if (capture_process_->state() != QProcess::NotRunning) {
            return;
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

    QStringList const args = {
        "-t", "jpeg", "-s", kPreviewScale, "-o", monitor_name_, "-"
    };
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

    screen_image_ = QString("data:image/jpeg;base64,%1").arg(QString(image_data.toBase64()));

    emit screen_image_changed();
}
