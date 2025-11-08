#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "backend/base.hpp"
#include "monitor_specs.hpp"

class BackendManager {
public:
    BackendManager();
    ~BackendManager() = default;

    void set_backend(BackendType type);

    // Configuration management
    void apply(const std::vector<MonitorSpecs> & monitors);
    void revert();

    // Apply with confirmation timer
    void apply_with_confirmation(
        const std::vector<MonitorSpecs> & monitors,
        const std::function<void()> & on_timeout,
        int timeout_seconds = 15
    );
    void confirm_apply();
    void cancel_apply();

private:
    std::unique_ptr<Backend> backend_;
    BackendType current_type_;
    bool pending_confirmation_;
};
