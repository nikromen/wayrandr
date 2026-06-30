#include "utils/helpers.hpp"

#include <asm-generic/socket.h>
#include <bits/types/struct_iovec.h>
#include <qassert.h>
#include <qcontainerfwd.h>
#include <qminmax.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qtypes.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.
#include <spdlog/spdlog.h>
#include <sys/pidfd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QPointer>
#include <QProcess>
#include <QRunnable>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QString>
#include <QThread>
#include <QThreadPool>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <exception>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
std::atomic_bool stopping{ false };
std::atomic_bool cleanup_failed{ false };
thread_local bool recovery_command = false;
int outstanding_jobs = 0;  // GUI thread only

auto system_error_message(int error = errno) -> std::string {
    return QString::fromLocal8Bit(std::strerror(error)).toStdString();
}

auto group_has_live_members(pid_t group) -> bool {
    const QDir proc("/proc");
    if (!proc.isReadable()) {
        throw std::runtime_error("cannot inspect /proc");
    }
    for (const auto & entry : proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool numeric = false;
        entry.toLongLong(&numeric);
        if (!numeric) {
            continue;
        }
        QFile stat(proc.filePath(entry + "/stat"));
        if (!stat.open(QIODevice::ReadOnly)) {
            if (!stat.exists()) {
                continue;
            }
            throw std::runtime_error("cannot read process state");
        }
        const auto data = stat.readAll().toStdString();
        const auto end = data.rfind(')');
        if (end == std::string::npos) {
            if (!stat.exists()) {
                continue;
            }
            throw std::runtime_error("invalid process state");
        }
        std::istringstream fields(data.substr(end + 1));
        char state = 0;
        pid_t parent = 0;
        pid_t process_group = 0;
        if (!(fields >> state >> parent >> process_group)) {
            throw std::runtime_error("invalid process group");
        }
        if (process_group == group && state != 'Z' && state != 'X' && state != 'x') {
            return true;
        }
    }
    return false;
}

// Transfer the leader's own pidfd before exec: Qt can reap it immediately afterwards.
struct GroupMessage {
    pid_t group = 0;
    alignas(cmsghdr) std::array<char, CMSG_SPACE(sizeof(int))> control{};
    iovec data{ &group, sizeof(group) };
    msghdr header{ nullptr, 0, &data, 1, control.data(), control.size(), 0 };
};

void capture_group_pidfd(QProcess & process, int socket) {
    process.setChildProcessModifier([&process, socket] {
        if (::setsid() == -1) {
            process.failChildProcessModifier("setsid", errno);
        }
        const int descriptor = static_cast<int>(::syscall(SYS_pidfd_open, ::getpid(), 0));
        if (descriptor == -1) {
            process.failChildProcessModifier("pidfd_open", errno);
        }
        GroupMessage message;
        message.group = ::getpid();
        auto * control = CMSG_FIRSTHDR(&message.header);
        control->cmsg_level = SOL_SOCKET;
        control->cmsg_type = SCM_RIGHTS;
        control->cmsg_len = CMSG_LEN(sizeof(descriptor));
        std::memcpy(CMSG_DATA(control), &descriptor, sizeof(descriptor));
        if (::sendmsg(socket, &message.header, MSG_NOSIGNAL) != sizeof(message.group)) {
            process.failChildProcessModifier("send pidfd", errno);
        }
        ::close(descriptor);
    });
}

auto receive_group_pidfd(int socket, pid_t & group) -> int {
    GroupMessage message;
    const auto received = ::recvmsg(socket, &message.header, MSG_DONTWAIT | MSG_CMSG_CLOEXEC);
    auto * control = CMSG_FIRSTHDR(&message.header);
    if (received != sizeof(message.group) || control == nullptr ||
        control->cmsg_level != SOL_SOCKET || control->cmsg_type != SCM_RIGHTS ||
        control->cmsg_len != CMSG_LEN(sizeof(int))) {
        throw std::runtime_error("process group pidfd not received");
    }
    int descriptor = -1;
    std::memcpy(&descriptor, CMSG_DATA(control), sizeof(descriptor));
    group = message.group;
    return descriptor;
}

void terminate_group(int descriptor, pid_t group) {
    if (::syscall(
            SYS_pidfd_send_signal, descriptor, SIGKILL, nullptr, PIDFD_SIGNAL_PROCESS_GROUP
        ) != 0) {
        if (errno == ESRCH) {
            return;  // The original group no longer exists.
        }
        throw std::runtime_error("process group signal failed: " + system_error_message());
    }
    QElapsedTimer deadline;
    deadline.start();
    while (group_has_live_members(group)) {
        if (deadline.elapsed() >= 1000) {
            throw std::runtime_error("live process group members remain");
        }
        QThread::msleep(5);
    }
}

auto command_string(const std::string & value) -> QString {
    const auto converted = QString::fromStdString(value);
    if (value.find('\0') != std::string::npos || converted.toStdString() != value) {
        throw std::invalid_argument("Program and arguments must be UTF-8 without NUL bytes");
    }
    return converted;
}

auto pool() -> QThreadPool & {
    static QThreadPool worker;
    worker.setMaxThreadCount(1);
    return worker;
}
}  // namespace

auto is_program_available(const std::string & program) -> bool {
    return !QStandardPaths::findExecutable(command_string(program)).isEmpty();
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
        qargs << command_string(arg);
    }
    // Use exactly the same resolver as availability detection, then start the
    // resolved path. PATH and the inherited environment remain user-controlled.
    const auto executable = QStandardPaths::findExecutable(command_string(program));
    if (executable.isEmpty()) {
        throw std::runtime_error(program + ": failed to start: executable not found");
    }
    QProcess process;
    process.setProgram(executable);
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
    if (stopping && !recovery_command) {
        throw std::runtime_error("Command cancelled at shutdown: " + program);
    }

    int sockets[2]{ -1, -1 };
    int group_descriptor = -1;
    const auto close_descriptors = qScopeGuard([&] {
        for (const int descriptor : { sockets[0], sockets[1], group_descriptor }) {
            if (descriptor >= 0) {
                ::close(descriptor);
            }
        }
    });
    if (::socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets) != 0) {
        throw std::runtime_error("cannot capture process group: " + system_error_message());
    }
    capture_group_pidfd(process, sockets[1]);

    spdlog::debug("Starting {} with {} arguments", program, args.size());
    process.start(QIODevice::ReadOnly);
    ::close(sockets[1]);
    sockets[1] = -1;

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
        if (failure.empty() && stopping && !recovery_command) {
            failure = "cancelled at shutdown";
        }
        if (failure.empty() && deadline.elapsed() >= options.timeout_ms) {
            failure = "timeout after " + std::to_string(options.timeout_ms) +
                " ms; operation may have partially completed";
        }
        if (!failure.empty()) {
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
    try {
        pid_t group = 0;
        // FailedToStart means no executable ran; a modifier failure may have sent no pidfd.
        if (process.error() != QProcess::FailedToStart) {
            group_descriptor = receive_group_pidfd(sockets[0], group);
            if (!failure.empty()) {
                terminate_group(group_descriptor, group);
            }
        }
    } catch (const std::exception & e) {
        cleanup_failed = true;
        failure += "; process group cleanup failed: " + std::string(e.what()) +
            "; further commands disabled";
    }
    if (process.state() != QProcess::NotRunning && !process.waitForFinished(1000)) {
        cleanup_failed = true;
        failure += "; process could not be reaped; further commands disabled";
    }
    drain();
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
        } else {
            spdlog::error("Recovery job rejected: command executor is shutting down");
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
    const bool recovery = receiver == nullptr;
    pool().start(
        QRunnable::create(
            [guard, recovery, work = std::move(work), failure = std::move(failure), settled] {
                Completion complete;
                std::string error;
                // Keep accepted recovery jobs in the serialized queue. Ordinary queued work
                // is skipped; an interrupted Apply retains its snapshot for the recovery.
                if (!stopping || recovery) {
                    recovery_command = recovery;
                    try {
                        complete = work();
                    } catch (const std::exception & e) {
                        error = e.what();
                    } catch (...) {
                        error = "Unknown worker failure";
                    }
                    recovery_command = false;
                }
                if (recovery && !error.empty()) {
                    // There is no GUI receiver, and its event loop may already have ended.
                    spdlog::error("Recovery job failed: {}", error);
                }
                QMetaObject::invokeMethod(
                    QCoreApplication::instance(),
                    [guard, complete = std::move(complete), failure, error, settled] {
                        --outstanding_jobs;
                        if (stopping) {
                            return;
                        }
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
            }
        ),
        priority
    );
}

void shutdown_commands() {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    if (stopping.exchange(true)) {
        return;
    }
    // A final worker fence follows all accepted jobs, including receiverless recovery.
    // Pump Qt events while commands finish; never wait for a process on the GUI thread.
    QEventLoop shutdown_loop;
    pool().start(
        QRunnable::create([&shutdown_loop] {
            QMetaObject::invokeMethod(&shutdown_loop, &QEventLoop::quit, Qt::QueuedConnection);
        }),
        std::numeric_limits<int>::min()
    );
    shutdown_loop.exec();
    // The fence has run: only worker bookkeeping remains, with no external commands.
    pool().waitForDone();
}
