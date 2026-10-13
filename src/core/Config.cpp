#include "core/Config.hpp"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace vds {

std::vector<std::pair<std::string, double*>> Config::fields() {
    std::vector<std::pair<std::string, double*>> table;

    auto add = [&table](const std::string& key, double& value) {
        table.emplace_back(key, &value);
    };
    auto add_set = [&add](const std::string& key, ThresholdSet& set) {
        add(key + ".caution", set.caution);
        add(key + ".warning", set.warning);
        add(key + ".critical", set.critical);
        add(key + ".hysteresis", set.hysteresis);
    };

    add_set("thresholds.inverter_temp", thresholds.inverter_temp);
    add_set("thresholds.motor_temp", thresholds.motor_temp);
    add_set("thresholds.pack_temp", thresholds.pack_temp);
    add_set("thresholds.soc", thresholds.soc);
    add_set("thresholds.tank_pressure", thresholds.tank_pressure);
    return table;
}

namespace {

std::string trim(const std::string& text) {
    const char* whitespace = " \t\r\n";
    const auto begin = text.find_first_not_of(whitespace);
    if (begin == std::string::npos) return "";
    const auto end = text.find_last_not_of(whitespace);
    return text.substr(begin, end - begin + 1);
}

}  // namespace

Config::LoadResult Config::loadFromString(const std::string& text) {
    LoadResult result;
    auto table = fields();
    std::istringstream input(text);
    std::string line;
    int line_number = 0;

    auto fail = [&](const std::string& message) {
        result.ok = false;
        result.errors.push_back("line " + std::to_string(line_number) + ": " + message);
    };

    while (std::getline(input, line)) {
        ++line_number;

        const auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        line = trim(line);
        if (line.empty()) continue;

        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            fail("expected 'key = value'");
            continue;
        }
        const std::string key = trim(line.substr(0, equals));
        const std::string value = trim(line.substr(equals + 1));

        double* target = nullptr;
        for (auto& entry : table) {
            if (entry.first == key) target = entry.second;
        }
        if (target == nullptr) {
            fail("unknown key '" + key + "'");
            continue;
        }

        char* end = nullptr;
        errno = 0;
        const double number = std::strtod(value.c_str(), &end);
        if (value.empty() || end == value.c_str() || *end != '\0' || errno != 0) {
            fail("invalid number '" + value + "' for key '" + key + "'");
            continue;
        }
        *target = number;
    }
    return result;
}

Config::LoadResult Config::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        LoadResult result;
        result.ok = false;
        result.errors.push_back("cannot open '" + path + "'");
        return result;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
}

}  // namespace vds
