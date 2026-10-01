#include "models/monitor_block.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qstringview.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include "utils/helpers.hpp"

namespace {
constexpr int K_PREVIEW_FPS = 1;
constexpr int K_PREVIEW_INTERVAL_MS = 1000 / K_PREVIEW_FPS;
constexpr char K_PREVIEW_SCALE[] = "0.2";
}  // namespace

MonitorBlock::MonitorBlock(QString monitor_name, QObject * parent)
    : QObject(parent),
      monitor_name_(std::move(monitor_name)),
      capture_timer_(new QTimer(this)),
      is_capturing_(false) {
    capture_timer_->setInterval(K_PREVIEW_INTERVAL_MS);
    connect(capture_timer_, &QTimer::timeout, this, &MonitorBlock::capture_frame);

    spdlog::debug(
        "Created MonitorBlock for: {} (preview {} FPS)", monitor_name_.toStdString(), K_PREVIEW_FPS
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

    if (capture_cancelled_) {
        *capture_cancelled_ = true;
    }

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
        return;
    }
    is_capturing_ = true;
    auto cancelled = std::make_shared<std::atomic_bool>(false);
    capture_cancelled_ = cancelled;
    const auto name = monitor_name_.toStdString();
    run_job(
        this,
        [this, name, cancelled] {
            CommandOptions options;
            options.cancelled = cancelled;
            options.timeout_ms = 1500;
            options.output_limit = 8 * 1024 * 1024;
            auto data = run_command(
                "grim", { "-t", "jpeg", "-s", K_PREVIEW_SCALE, "-o", name, "-" }, options
            );
            return [this, data = std::move(data), cancelled] {
                is_capturing_ = false;
                if (*cancelled || data.empty()) {
                    return;
                }
                const auto bytes = QByteArray::fromStdString(data);
                screen_image_ = QString("data:image/jpeg;base64,%1").arg(QString(bytes.toBase64()));
                emit screen_image_changed();
            };
        },
        [this, cancelled](const std::string & error) {
            is_capturing_ = false;
            if (!*cancelled) {
                spdlog::warn("Grim capture failed: {}", error);
            }
        },
        {},
        -1
    );
}
