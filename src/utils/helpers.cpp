#include "utils/helpers.hpp"

#include <qassert.h>
#include <qcontainerfwd.h>
#include <qminmax.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qtypes.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPointer>
#include <QProcess>
#include <QRunnable>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>
#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
std::atomic_bool stopping{ false };
std::atomic_bool cleanup_failed{ false };
int outstanding_jobs = 0;  // GUI thread only

auto pool() -> QThreadPool & {
    static QThreadPool worker;
    worker.setMaxThreadCount(1);
    return worker;
}
}  // namespace

auto is_program_available(const std::string & program) -> bool {
    return !QStandardPaths::findExecutable(QString::fromStdString(program)).isEmpty();
}

auto run_command(
    const std::string & program, const std::vector<std::string> & args, CommandOptions options
) -> std::string {
    if ((QCoreApplication::instance() != nullptr) &&
        QThread::currentThread() == QCoreApplication::instance()->thread()) {
        throw std::logic_error("External command requested on GUI thread: " + program);
    }
    if (options.timeout_ms <= 0 || options.output_limit == 0) {
        throw std::invalid_argument("Invalid command limits");
    }

    QStringList qargs;
    for (const auto & arg : args) {
        qargs << QString::fromStdString(arg);
    }
    QProcess process;
    // Give the command and its descendants a private process group.
    process.setChildProcessModifier([] {
        if (::setsid() == -1) {
            ::_exit(127);
        }
    });
    process.setProgram(QString::fromStdString(program));
    process.setArguments(qargs);
    QByteArray output;
    QByteArray errors;
    std::string failure;
    QElapsedTimer deadline;
    deadline.start();

    if (options.cancelled && *options.cancelled) {
        throw std::runtime_error("Command cancelled: " + program);
    }
    if (cleanup_failed) {
        throw std::runtime_error(
            "Command executor disabled after failed process cleanup: " + program
        );
    }
    if (stopping) {
        throw std::runtime_error("Command cancelled at shutdown: " + program);
    }

    spdlog::debug("Starting {} with {} arguments", program, args.size());
    process.start(QIODevice::ReadOnly);

    // Short waits are confined to the worker. Drain both pipes on every wakeup;
    // never retain truncated stdout as a valid JSON response or image.
    auto drain = [&] {
        const auto out = process.readAllStandardOutput();
        const auto err = process.readAllStandardError();
        const auto used = static_cast<size_t>(output.size() + errors.size());
        if (used + static_cast<size_t>(out.size() + err.size()) > options.output_limit) {
            if (failure.empty()) {
                failure =
                    "output limit exceeded (" + std::to_string(options.output_limit) + " bytes)";
            }
            // Keep a bounded diagnostic even when stdout floods the pipe.
            const auto room =
                qMin<size_t>(options.output_limit - used, 4096 - qMin<size_t>(4096, errors.size()));
            errors.append(err.left(static_cast<qsizetype>(room)));
        } else {
            output.append(out);
            errors.append(err);
        }
    };

    while (process.state() != QProcess::NotRunning) {
        process.waitForReadyRead(20);
        drain();
        if (failure.empty() && options.cancelled && *options.cancelled) {
            failure = "cancelled";
        }
        if (failure.empty() && stopping) {
            failure = "cancelled at shutdown";
        }
        if (failure.empty() && deadline.elapsed() >= options.timeout_ms) {
            failure = "timeout after " + std::to_string(options.timeout_ms) +
                " ms; operation may have partially completed";
        }
        if (!failure.empty()) {
            const auto pid = process.processId();
            if (pid > 0) {
                ::kill(-static_cast<pid_t>(pid), SIGKILL);
            }
            process.kill();
            if (!process.waitForFinished(1000) && process.state() != QProcess::NotRunning) {
                // Never let another mutation overlap a process whose death is
                // unconfirmed. QProcess performs its final cleanup on this worker.
                cleanup_failed = true;
                failure += "; process could not be reaped; further commands disabled";
            }
            break;
        }
    }

    drain();
    if (failure.empty() && process.error() == QProcess::FailedToStart) {
        failure = "failed to start: " + process.errorString().toStdString();
    }
    if (failure.empty() && process.exitStatus() == QProcess::CrashExit) {
        failure = "crashed (exit code " + std::to_string(process.exitCode()) + ")";
    }
    if (failure.empty() && process.exitCode() != 0) {
        failure = "exit code " + std::to_string(process.exitCode());
    }
    if (!failure.empty()) {
        std::string message =
            program + ": " + failure + "; stderr: " + errors.left(4096).toStdString();
        spdlog::error("{}", message);
        throw std::runtime_error(message);
    }
    if (!errors.isEmpty()) {
        spdlog::debug("{} stderr: {}", program, errors.left(4096).toStdString());
    }

    return output.toStdString();
}

void run_job(
    QObject * receiver,
    std::function<Completion()> work,
    std::function<void(const std::string &)> failure,
    const Completion & settled,
    int priority
) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    if (stopping) {
        if (settled) {
            settled();
        }
        if (receiver != nullptr) {
            failure("Command executor is shutting down");
        }
        return;
    }
    if ((receiver != nullptr) && outstanding_jobs >= 64) {
        if (settled) {
            settled();
        }
        if (receiver != nullptr) {
            failure("Command queue is full");
        }
        return;
    }

    ++outstanding_jobs;
    const QPointer<QObject> guard(receiver);
    pool().start(
        QRunnable::create([guard, work = std::move(work), failure = std::move(failure), settled] {
            Completion complete;
            std::string error;
            try {
                complete = work();
            } catch (const std::exception & e) {
                error = e.what();
            } catch (...) {
                error = "Unknown worker failure";
            }
            QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                [guard, complete = std::move(complete), failure, error, settled] {
                    --outstanding_jobs;
                    if (settled) {
                        settled();
                    }
                    if (!guard) {
                        return;
                    }
                    if (!error.empty()) {
                        failure(error);
                    } else if (complete) {
                        try {
                            complete();
                        } catch (const std::exception & e) {
                            if (guard) {
                                failure(e.what());
                            }
                        }
                    }
                },
                Qt::QueuedConnection
            );
        }),
        priority
    );
}

void shutdown_commands() {
    stopping = true;
    pool().clear();
    pool().waitForDone();
}
