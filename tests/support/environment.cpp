#include "support/environment.hpp"

#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <qtestsupport_core.h>

#include <QCoreApplication>
#include <QFile>
#include <QTest>
#include <clocale>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utils/helpers.hpp"

void require(bool condition, const char * message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void drain_jobs() {
    auto state = std::make_shared<JobResult>();
    QObject receiver;
    run_job(
        &receiver,
        [state] { return [state] { state->done = true; }; },
        [state](const std::string & error) {
            state->error = error;
            state->done = true;
        },
        {},
        -2
    );
    // Cleanup must complete even when an assertion returned early. Commands already
    // have production deadlines; the fence runs after queued capture/rollback work.
    while (!state->done) {
        QTest::qWait(5);
    }
}

Environment::Environment()
    : locale_(std::setlocale(LC_NUMERIC, nullptr)) {
    require(dir.isValid(), "Temporary directory unavailable");
    for (const auto * name :
         { "PATH", "WAYRANDR_TEST_DIR", "XDG_CONFIG_HOME", "XDG_RUNTIME_DIR", "WAYLAND_DISPLAY" }) {
        variables_.emplace(name, std::make_pair(qEnvironmentVariableIsSet(name), qgetenv(name)));
    }
    qputenv("PATH", dir.path().toUtf8());
    qputenv("WAYRANDR_TEST_DIR", dir.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", dir.path().toUtf8());
    qputenv("XDG_RUNTIME_DIR", dir.path().toUtf8());
}

Environment::~Environment() {
    drain_jobs();
    std::setlocale(LC_NUMERIC, locale_.c_str());
    for (const auto & entry : variables_) {
        if (entry.second.first) {
            qputenv(entry.first.constData(), entry.second.second);
        } else {
            qunsetenv(entry.first.constData());
        }
    }
}

void Environment::install(const QString & source, const QString & name) const {
    require(
        QFile::copy(source, dir.filePath(name)),  // NOLINT: QFile::copy.
        "Cannot copy fake program"
    );
    require(
        QFile::setPermissions(
            dir.filePath(name), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
        ),
        "Fake program permissions"
    );
}

auto start_command(
    QObject * receiver,
    const std::string & program,
    const std::vector<std::string> & args,
    const CommandOptions & options
) -> std::shared_ptr<JobResult> {
    auto state = std::make_shared<JobResult>();
    run_job(
        receiver,
        [state, program, args, options] {
            auto output = run_command(program, args, options);
            return [state, output] {
                state->output = output;
                state->done = true;
            };
        },
        [state](const std::string & error) {
            state->error = error;
            state->done = true;
        }
    );
    return state;
}

void BackendReport::verify(bool condition, const char * message) {
    observations.push_back({ condition, message });
    if (!condition) {
        throw std::runtime_error(message);
    }
}
