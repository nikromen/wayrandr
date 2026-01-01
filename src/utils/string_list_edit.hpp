#pragma once

#include <string>
#include <vector>

namespace string_list_edit {

inline void add(std::vector<std::string> & items, const std::string & value) {
    items.push_back(value);
}

inline void remove_at(std::vector<std::string> & items, int index) {
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return;
    }
    items.erase(items.begin() + index);
}

inline void set_at(std::vector<std::string> & items, int index, const std::string & value) {
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return;
    }
    items[static_cast<size_t>(index)] = value;
}

}  // namespace string_list_edit
