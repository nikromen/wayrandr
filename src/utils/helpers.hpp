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

// Resolve with the same Qt/PATH lookup as is_program_available(); no shell parsing.
// Program and arguments must be valid UTF-8 without embedded NUL bytes.
// Capture the leader's pidfd before exec and signal the private group
// through PIDFD_SIGNAL_PROCESS_GROUP, never a recycled numeric PID.
// Successful commands may leave legitimate descendants running; they are not killed.
// Descendants that leave the private process group are outside this cleanup.
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
//   no result delivery. Such jobs are recovery: accepted jobs survive shutdown,
//   including their external commands. Do not use nullptr for ordinary work.
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
// that limit. Shutdown rejects new jobs, cancels running ordinary commands and skips
// queued ordinary work. Accepted receiverless recovery runs on the same worker after
// the interrupted operation, with normal command deadlines and cleanup protections.
// Recovery errors are logged on the worker even without a GUI receiver. During shutdown
// GUI result delivery (including settled) is suppressed.
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
// Call on the GUI thread after destroying GUI owners so their recovery is queued.
// Stop ordinary work, finish accepted recovery, then join the worker. Pumps a local
// Qt event loop until the worker fence finishes; the GUI thread never waits on a
// process. QCoreApplication must remain alive. Repeated calls after completion are safe.
void shutdown_commands();
