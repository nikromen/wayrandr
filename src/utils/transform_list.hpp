#pragma once

#include <QStringList>
#include <string>

#include "monitor_specs.hpp"

namespace transform_list {

[[nodiscard]] inline auto as_qstring_list() -> QStringList {
    QStringList transforms;
    for (const std::string & transform : transform_utils::all_strings()) {
        transforms.append(QString::fromStdString(transform));
    }
    return transforms;
}

}  // namespace transform_list
