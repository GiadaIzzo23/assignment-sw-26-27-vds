#include <cstddef>
#include <set>
#include <string>

#include "check.hpp"
#include "core/Config.hpp"

using namespace vds;

namespace {

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

}  // namespace

TEST(default_thresholds_are_ordered_by_severity) {
    const Config cfg;
    // Rising signals: the higher the value, the worse.
    for (const ThresholdSet* set : {&cfg.thresholds.inverter_temp, &cfg.thresholds.motor_temp,
                                    &cfg.thresholds.pack_temp}) {
        CHECK(set->caution < set->warning);
        CHECK(set->warning < set->critical);
        CHECK(set->hysteresis > 0);
    }
    // Falling signals: the lower the value, the worse.
    for (const ThresholdSet* set : {&cfg.thresholds.soc, &cfg.thresholds.tank_pressure}) {
        CHECK(set->caution > set->warning);
        CHECK(set->warning > set->critical);
        CHECK(set->hysteresis > 0);
    }
}

TEST(every_field_key_is_unique) {
    Config cfg;
    std::set<std::string> keys;
    for (const auto& entry : cfg.fields()) keys.insert(entry.first);
    CHECK_EQ(keys.size(), cfg.fields().size());
}

TEST(load_overrides_values_and_ignores_comments_and_blank_lines) {
    Config cfg;
    const auto result = cfg.loadFromString(
        "# a full-line comment\n"
        "\n"
        "thresholds.soc.critical = 12   # trailing comment\n"
        "   thresholds.inverter_temp.warning=90\n");
    CHECK(result.ok);
    CHECK(result.errors.empty());
    CHECK_NEAR(cfg.thresholds.soc.critical, 12.0, 1e-12);
    CHECK_NEAR(cfg.thresholds.inverter_temp.warning, 90.0, 1e-12);
    // Everything that was not mentioned keeps its default.
    CHECK_NEAR(cfg.thresholds.soc.warning, 20.0, 1e-12);
}

TEST(load_reports_errors_with_line_numbers_but_keeps_going) {
    Config cfg;
    const auto result = cfg.loadFromString(
        "thresholds.soc.critical = 12\n"       // line 1: valid
        "thresholds.nope.critical = 5\n"       // line 2: unknown key
        "thresholds.soc.warning = abc\n"       // line 3: not a number
        "this line has no equals sign\n"       // line 4: malformed
        "thresholds.soc.caution = 35\n");      // line 5: valid
    CHECK(!result.ok);
    CHECK_EQ(result.errors.size(), static_cast<std::size_t>(3));
    CHECK(contains(result.errors[0], "line 2"));
    CHECK(contains(result.errors[1], "line 3"));
    CHECK(contains(result.errors[2], "line 4"));
    // The valid lines before and after the errors were still applied.
    CHECK_NEAR(cfg.thresholds.soc.critical, 12.0, 1e-12);
    CHECK_NEAR(cfg.thresholds.soc.caution, 35.0, 1e-12);
    // The line with the bad number must not have changed the value.
    CHECK_NEAR(cfg.thresholds.soc.warning, 20.0, 1e-12);
}

TEST(a_number_followed_by_garbage_is_rejected) {
    Config cfg;
    const auto result = cfg.loadFromString("thresholds.soc.critical = 12x\n");
    CHECK(!result.ok);
    CHECK_NEAR(cfg.thresholds.soc.critical, 10.0, 1e-12);
}

TEST(loading_a_missing_file_reports_an_error) {
    Config cfg;
    const auto result = cfg.loadFromFile("/definitely/not/here.cfg");
    CHECK(!result.ok);
    CHECK_EQ(result.errors.size(), static_cast<std::size_t>(1));
}

TEST(the_shipped_config_file_matches_the_built_in_defaults) {
    Config defaults;
    Config loaded;
    const auto result = loaded.loadFromFile(std::string(VDS_SOURCE_DIR) + "/config/vds.cfg");
    CHECK(result.ok);
    for (const auto& error : result.errors) std::printf("    %s\n", error.c_str());

    const auto default_fields = defaults.fields();
    const auto loaded_fields = loaded.fields();
    CHECK_EQ(default_fields.size(), loaded_fields.size());
    for (std::size_t i = 0; i < default_fields.size(); ++i) {
        CHECK_NEAR(*default_fields[i].second, *loaded_fields[i].second, 1e-12);
    }
}
