#pragma once

#include <QObject>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Synchronous worker API: never call from the GUI thread. Empty stdout is success.
struct CommandOptions {
    std::shared_ptr<std::atomic_bool> cancelled;
    int timeout_ms = 5000;
    size_t output_limit = 4 * 1024 * 1024;  // Combined stdout + stderr, binary safe.
};

auto run_command(
    const std::string & program, const std::vector<std::string> & args, CommandOptions options = {}
) -> std::string;
[[nodiscard]] auto is_program_available(const std::string & program) -> bool;

// A callback with no arguments or return value, used to deliver a result to the GUI.
using Completion = std::function<void()>;

// Queue work on a single shared worker thread without blocking the GUI.
// Call run_job() from the GUI thread while QCoreApplication is alive.
// This schedules arbitrary work; external processes are started by run_command()
// inside work, not by run_job() itself.
//
// receiver:
//   The QObject receiving the result, usually the caller's `this`. Its lifetime
//   is tracked with QPointer. If it is destroyed, completion and failure are
//   skipped, but queued/running work is NOT cancelled. nullptr allows work with
//   no result delivery, such as recovery after a window has been destroyed.
// work:
//   Runs on the worker thread and returns a Completion callback. Capture input
//   data by value and retain ownership of objects needed by the worker. Never
//   access GUI objects here; their pointers may only be carried into the returned
//   callback. That callback runs on the GUI thread if receiver still exists.
//   Returning an empty Completion is valid when no result delivery is needed.
// failure:
//   Runs on the GUI thread if receiver exists. Receives the exception message
//   from work (or a fallback for an unknown exception), or a std::exception from
//   the completion callback. Also handles rejection during shutdown or when the
//   queue is full. Supply a callable callback whenever receiver is non-null.
// settled:
//   Optional shared cleanup on the GUI thread, before completion or failure.
//   Runs even if receiver was destroyed, so it must not depend on that object's
//   lifetime. For example, capture a shared_ptr to the manager to clear its busy
//   flag. The callback is copied into the job; the reference is not retained.
//   Empty by default. settled and failure must not throw.
// priority:
//   Higher numbers run before lower numbers among waiting jobs. 0 (normal work,
//   Apply/rollback) takes precedence over -1 (preview captures). Equal priorities
//   are FIFO. Priority does NOT interrupt work that is already running.
//
// Normal delivery order: work on worker -> settled on GUI -> completion OR failure
// on GUI. Rejection calls settled and then failure immediately within run_job().
// Ordinary jobs are rejected at 64 outstanding jobs; nullptr recovery jobs bypass
// that limit. During shutdown queued jobs are discarded, and results from running
// work may no longer be delivered if the GUI event loop has stopped. settled is
// therefore not guaranteed for every submitted job during application shutdown.
//
// Example: the outer lambda gathers data on the worker; the returned lambda uses
// it on the GUI thread. Capturing `this` outside only passes it to the callback.
//   run_job(this,
//       [this] {
//           auto specs = get_monitor_specs_list();
//           return [this, specs = std::move(specs)]() mutable {
//               initialize_monitors(std::move(specs));
//           };
//       },
//       [this](const std::string & error) { show_error(error); });
void run_job(
    QObject * receiver,
    std::function<Completion()> work,
    std::function<void(const std::string &)> failure,
    const Completion & settled = {},
    int priority = 0
);
// Stop accepting work, cancel running commands and join the worker at shutdown.
void shutdown_commands();
