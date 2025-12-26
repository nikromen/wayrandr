#include "utils/helpers.hpp"

#include <spdlog/spdlog.h>

#include <QProcess>
#include <QStringList>
#include <sstream>
#include <stdexcept>
#include <string>

auto run_command(const std::string & program, const std::vector<std::string> & args) -> std::string {
    QStringList qargs;
    for (const auto & arg : args) {
        qargs << QString::fromStdString(arg);
    }

    std::ostringstream cmd_log;
    cmd_log << program;
    for (const auto & arg : args) {
        cmd_log << ' ' << arg;
    }
    spdlog::debug("Running command: {}", cmd_log.str());

    QProcess process;
    process.setProgram(QString::fromStdString(program));
    process.setArguments(qargs);
    process.start(QIODevice::ReadOnly);

    if (!process.waitForFinished(-1)) {
        spdlog::error("Failed to start command: {}", program);
        throw std::runtime_error("QProcess::start failed for: " + program);
    }

    const QString stderr_output = QString::fromUtf8(process.readAllStandardError());
    const QString stdout_output = QString::fromUtf8(process.readAllStandardOutput());

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        spdlog::error(
            "Command {} exited with code {}: {}",
            program,
            process.exitCode(),
            stderr_output.toStdString()
        );
        throw std::runtime_error(
            "Command failed: " + program + " - " + stderr_output.toStdString()
        );
    }

    const std::string output = stdout_output.toStdString();
    spdlog::debug("Command output: {} bytes", output.size());
    return output;
}
