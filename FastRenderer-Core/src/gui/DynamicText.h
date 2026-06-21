#pragma once
#include <string>
#include <map>

namespace DynamicText {

inline std::map<std::string, std::string> values;

inline const std::string& get(const std::string& key) {
    static std::string empty;
    auto it = values.find(key);
    return (it != values.end()) ? it->second : empty;
}

inline void set(const std::string& key, const std::string& value) {
    values[key] = value;
}

}
