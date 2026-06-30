#pragma once

#include <linux/prctl.h>
#include <qfiledevice.h>
#include <qtestsupport_core.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QString>
#include <QTest>
#include <atomic>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <utility>

#include "support/environment.hpp"

// Own/reap orphaned fake descendants, including on an early Qt assertion return.
// A child owned by this test cannot have its PID reused until we reap it.
struct ProcessTreeCleanup {
    explicit ProcessTreeCleanup(std::shared_ptr<std::atomic_bool> cancelled = {})
        : cancelled_(std::move(cancelled)) {
        require(::prctl(PR_GET_CHILD_SUBREAPER, &previous_) == 0, "Cannot inspect subreaper");
        require(::prctl(PR_SET_CHILD_SUBREAPER, 1) == 0, "Cannot adopt test descendants");
    }

    ~ProcessTreeCleanup() {
        if (cancelled_) {
            *cancelled_ = true;
        }
        drain_jobs();
        QElapsedTimer deadline;
        deadline.start();
        bool children = true;
        while (children && deadline.elapsed() < 2000) {
            children = false;
            const QDir tasks(QStringLiteral("/proc/self/task"));
            for (const auto & task : tasks.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                QFile file(tasks.filePath(task + "/children"));
                if (!file.open(QIODevice::ReadOnly)) {
                    continue;
                }
                std::istringstream ids(file.readAll().toStdString());
                pid_t pid = 0;
                while (ids >> pid) {
                    const auto waited = ::waitpid(pid, nullptr, WNOHANG);
                    if (waited == 0) {
                        children = true;
                        ::kill(pid, SIGKILL);
                    }
                }
            }
            if (children) {
                QTest::qWait(5);
            }
        }
        ::prctl(PR_SET_CHILD_SUBREAPER, previous_);
    }

    ProcessTreeCleanup(const ProcessTreeCleanup &) = delete;
    auto operator=(const ProcessTreeCleanup &) -> ProcessTreeCleanup & = delete;

private:
    int previous_ = 0;
    std::shared_ptr<std::atomic_bool> cancelled_;
};

inline auto process_is_live(pid_t pid) -> bool {
    QFile file(QStringLiteral("/proc/%1/stat").arg(pid));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const auto stat = file.readAll();
    const auto end_name = stat.lastIndexOf(')');
    if (end_name < 0 || end_name + 2 >= stat.size()) {
        return false;
    }
    const auto state = stat.at(end_name + 2);
    return state != 'Z' && state != 'X' && state != 'x';
}
