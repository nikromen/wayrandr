#pragma once
#include <qfiledevice.h>
#include <qtenvironmentvariables.h>
#include <qtypes.h>

#include <QDir>
#include <QFile>
#include <QString>
#include <clocale>
#include <nlohmann/json.hpp>  // NOLINT: JSON definitions are required here.
#include <string>

#include "monitor_specs.hpp"
#include "nlohmann/json_fwd.hpp"
#include "support/environment.hpp"
using json = nlohmann::json;

struct Fixture : Environment {
    Fixture() {
        require(dir.isValid(), "Temporary directory unavailable");
        require(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        install(FAKE_WLR_CLI, "wlr-randr");
        reset();
    }

    void write(const char * name, const std::string & value) const {
        QFile file(dir.filePath(name));
        require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot write fixture");
        file.write(value.data(), static_cast<qint64>(value.size()));
    }

    [[nodiscard]] auto read(const char * name) const -> std::string {
        QFile file(dir.filePath(name));
        require(file.open(QIODevice::ReadOnly), "Cannot read fixture");
        return file.readAll().toStdString();
    }

    [[nodiscard]] json state() const { return json::parse(read("state")); }

    void reset() const {
        write(
            "state", R"([{"name":"DP-1","description":"Test","enabled":true,
            "physical_size":{"width":300,"height":200},"position":{"x":0,"y":0},
            "scale":1,"transform":"normal","adaptive_sync":false,
            "modes":[{"width":1920,"height":1080,"refresh":60,"preferred":true,"current":true}]}])"
        );
        write("calls", "");
        write("failures", "0");
        write("hangs", "0");
        QFile::remove(dir.filePath("query_failure"));
    }

    void multi_monitor() const {
        auto monitors = state();
        monitors[0]["modes"].push_back(
            { { "width", 1280 },
              { "height", 720 },
              { "refresh", 75 },
              { "preferred", false },
              { "current", false } }
        );
        auto second = monitors[0];
        second["name"] = "DP-2";
        second["enabled"] = false;
        second["position"] = { { "x", 2400 }, { "y", 100 } };
        monitors.push_back(second);
        write("state", monitors.dump());
    }

    void profiles() const {
        require(QDir().mkpath(dir.filePath("kanshi")), "Kanshi config directory");
        require(QDir().mkpath(dir.filePath("auto-wlr-randr")), "Auto config directory");
        write("kanshi/config", R"(include "$XDG_CONFIG_HOME/kanshi/extra.conf"
output HDMI-1 disable
output "Example Model Serial" alias $desk
profile desk {
    output DP-1 mode 1920x1080@60Hz position 0,0 scale 1
}
profile spare {
    output DP-1 disable
}
)");
        write("kanshi/extra.conf", "# Included defaults\n");
        write("auto-wlr-randr/config.toml", R"(on_no_match_exec = ["notify-send unmatched"]
[profile.desk]
exec = ["notify-send desk"]
[[profile.desk.settings]]
output = "DP-1"
mode = "1920x1080@60Hz"
scale = 1.0
right_of = "HDMI-1"
[profile.spare]
[[profile.spare.settings]]
output = "DP-1"
on = false
)");
        write("daemon_mode", "success");
        write("daemon_calls", "");
        require(QDir().mkpath(dir.filePath("bin")), "Fake binary directory");
        qputenv("PATH", (dir.filePath("bin") + ":" + dir.path()).toUtf8());
        for (const char * name : { "kanshi", "kanshictl", "auto-wlr-randr", "auto-wlr-randrctl" }) {
            install(FAKE_DAEMON, QString("bin/%1").arg(name));
        }
    }

    auto target() {
        auto monitors = get_monitor_specs_list();
        monitors[0].get_enabled_monitor_settings()->set_scale(2);
        return monitors;
    }
};
