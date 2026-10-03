#pragma once
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/helpers.hpp"

void require(bool condition, const char * message);

// Drain queued work before restoring globals or removing temporary files.
void drain_jobs();

struct JobDrain {
    ~JobDrain() { drain_jobs(); }
};

struct Environment {
    QTemporaryDir dir;
    Environment();
    ~Environment();
    Environment(const Environment &) = delete;
    auto operator=(const Environment &) -> Environment & = delete;
    void install(const QString & source, const QString & name) const;

private:
    std::map<QByteArray, std::pair<bool, QByteArray>> variables_;
    std::string locale_;
};

// Shared state survives assertion returns and receiver destruction. Only GUI completions
// mutate these fields. Tests report all worker results on the GUI thread.
struct JobResult {
    bool done = false;
    std::string error;
    std::string output;
};

auto start_command(
    QObject * receiver,
    const std::string & program,
    const std::vector<std::string> & args,
    const CommandOptions & options = {}
) -> std::shared_ptr<JobResult>;

struct Observation {
    bool passed;
    std::string message;
};

struct BackendReport {
    std::vector<Observation> observations;
    void verify(bool condition, const char * message);

    template <typename Function>
    void throws(Function function) {
        bool failed = false;
        try {
            function();
        } catch (const std::exception &) {
            failed = true;
        }
        verify(failed, "Expected an exception");
    }
};
