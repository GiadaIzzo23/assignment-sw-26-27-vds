#include "core/Config.hpp"
#include "ui/Pages.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <utility>

#include "ui/WarningPolicy.hpp"

namespace vds {
namespace {

constexpr int kW = 1280;
constexpr int kH = 720;
constexpr int kLine = 25;

constexpr Color kText{235, 235, 235};
constexpr Color kDim{130, 138, 155};
constexpr Color kHead{110, 170, 255};
constexpr Color kGreen{80, 220, 120};
constexpr Color kPanel{26, 30, 40};
constexpr Color kBlack{0, 0, 0};

template <typename... Args>
std::string fmt(const char* f, Args... a) {
    char b[96];
    std::snprintf(b, sizeof b, f, a...);
    return b;
}

Color colorFor(Severity s) {
    switch (s) {
        case Severity::Caution: return {255, 200, 0};
        case Severity::Warning: return {255, 120, 0};
        case Severity::Critical: return {235, 35, 45};
        default: return kText;
    }
}
Color okOr(Severity s, Color ok) { return s == Severity::None ? ok : colorFor(s); }
Color onColor(Severity s) { return s == Severity::Critical ? Color{255, 255, 255} : kBlack; }

void centered(Gfx& g, int cx, int y, const std::string& s, int size, Color c) {
    g.text(cx - g.textWidth(s, size) / 2, y, s, size, c);
}

void bar(Gfx& g, int x, int y, int w, int h, double frac, Color c) {
    frac = std::clamp(frac, 0.0, 1.0);
    g.fillRect(x, y, w, h, {35, 38, 48});
    g.fillRect(x, y, static_cast<int>(w * frac), h, c);
    g.outlineRect(x, y, w, h, kDim);
}

// Banner (worst warning that the policy wants as a banner) + chips + flashing frame.
void drawAlerts(Gfx& g, Mode mode, const VehicleState& s, const WarningEngine& w, int bannerY,
                int chipY) {
    const bool blink = flashOn(s.time_s);
    const Warning* banner = nullptr;
    bool flashing = false;
    int extra = 0;
    int chipX = 20;

    for (const Warning& wr : w.active()) {  // sorted: worst first
        const Presentation p = presentationFor(mode, wr.id, wr.severity);
        if (p == Presentation::Banner || p == Presentation::FlashBanner) {
            if (!banner) {
                banner = &wr;
                flashing = (p == Presentation::FlashBanner);
            } else {
                ++extra;
            }
        } else if (p == Presentation::Chip) {
            const std::string label = describe(wr);
            const int cw = g.textWidth(label, 22) + 20;
            if (chipX + cw > kW - 20) continue;
            g.fillRect(chipX, chipY, cw, 34, colorFor(wr.severity));
            g.text(chipX + 10, chipY + 4, label, 22, onColor(wr.severity));
            chipX += cw + 10;
        }
    }

    if (banner) {
        Color fill = colorFor(banner->severity);
        const bool dark = flashing && !blink;
        if (dark) fill = {70, 10, 14};
        g.fillRect(0, bannerY, kW, 80, fill);
        std::string msg = describe(*banner);
        if (extra > 0) msg += "  +" + std::to_string(extra);
        centered(g, kW / 2, bannerY + 14, msg, 46, dark ? Color{255, 255, 255} : onColor(banner->severity));
        if (flashing && blink)
            for (int i = 0; i < 6; ++i) g.outlineRect(i, i, kW - 2 * i, kH - 2 * i, fill);
    }
}

// Simulator chrome (not part of the car's dashboard): mode tabs + key hints.
void drawFooter(Gfx& g, Mode mode) {
    const char* names[3] = {"1 DEBUG", "2 MANUAL", "3 DRIVERLESS"};
    const Mode modes[3] = {Mode::Debugging, Mode::Manual, Mode::Driverless};
    int x = 20;
    for (int i = 0; i < 3; ++i) {
        const bool active = modes[i] == mode;
        const int w = g.textWidth(names[i], 18) + 24;
        g.fillRect(x, 686, w, 28, active ? kHead : kPanel);
        g.text(x + 12, 689, names[i], 18, active ? kBlack : kDim);
        x += w + 8;
    }
    const char* hint = "";
    switch (mode) {
        case Mode::Debugging: hint = "F1-F7 inject fault   F8 clear   F9 reset"; break;
        case Mode::Manual: hint = "W/S pedals  A/D steer  T tractive  Enter drive  Z/X power  C/V regen"; break;
        case Mode::Driverless: hint = "M ASMS  Q/E mission  G go  Backspace e-stop  F1-F7 faults"; break;
    }
    g.text(kW - 20 - g.textWidth(hint, 18), 689, hint, 18, kDim);
}

// ----------------------------------------------------------------- DEBUGGING
struct Column {
    Gfx& g;
    int x;
    int y;
    void head(const char* t) {
        g.text(x, y, t, 22, kHead);
        y += kLine + 4;
    }
    void kv(const char* label, const std::string& v, Color c = kText) {
        g.text(x, y, label, 20, kDim);
        g.text(x + 150, y, v, 20, c);
        y += kLine;
    }
    void gap() { y += 10; }
};

// A titled box: background + heading. The caller lays the rows out inside it.
void panel(Gfx& g, int x, int y, int w, int h, const char* title) {
    g.fillRect(x, y, w, h, kPanel);
    g.text(x + 14, y + 5, title, 20, kHead);
}

void drawDebugging(Gfx& g, const VehicleState& s, const WarningEngine& w) {
    static const char* kInv[kInverterCount] = {"FL", "FR", "RL", "RR"};
    g.text(20, 8, fmt("DEBUGGING   t = %.1f s", s.time_s), 28, kText);

    constexpr int kTop = 88;   // below the chip row
    constexpr int kBody = 32;  // from the top of a panel to its first row

    // ---- column 1: vehicle, settings
    panel(g, 10, kTop, 410, 215, "VEHICLE");
    Column v{g, 24, kTop + kBody};
    v.kv("speed", fmt("%.1f km/h", s.speed_kmh));
    v.kv("target speed", fmt("%.1f km/h", s.target_speed_kmh));
    v.kv("throttle", fmt("%.0f %%", s.throttle * 100));
    v.kv("brake", fmt("%.0f %%", s.brake * 100));
    v.kv("steering", fmt("%+.1f deg", s.steering_deg));
    v.kv("distance", fmt("%.0f m", s.distance_m));
    v.kv("lap", fmt("%d", s.lap));

    panel(g, 10, kTop + 223, 410, 190, "SETTINGS");
    Column st{g, 24, kTop + 223 + kBody};
    st.kv("power level", fmt("%d", s.power_level));
    st.kv("regen level", fmt("%d", s.regen_level));
    st.kv("torque vect.", s.tv_level == 0 ? "OFF" : fmt("%d", s.tv_level));
    st.kv("DRS", fmt("req %s / %s", s.drs_requested ? "Y" : "N", s.drs_open ? "OPEN" : "SHUT"));
    st.kv("TS state", toString(s.ts));
    st.kv("LV voltage", fmt("%.2f V", s.lv_voltage));

    // ---- column 2: accumulator, ECUs
    panel(g, 430, kTop, 390, 165, "ACCUMULATOR");
    Column a{g, 444, kTop + kBody};
    a.kv("SoC", fmt("%.1f %%", s.soc_pct), okOr(w.severityOf(WarnId::LowSoc), kText));
    a.kv("pack voltage", fmt("%.1f V", s.pack_voltage));
    a.kv("pack current", fmt("%.1f A", s.pack_current_a));
    a.kv("pack power", fmt("%.1f kW", s.pack_power_kw));
    a.kv("pack temp", fmt("%.1f C", s.pack_temp_c), okOr(w.severityOf(WarnId::PackTemp), kText));

    panel(g, 430, kTop + 173, 390, 165, "ECU STATUS");
    Column e{g, 444, kTop + 173 + kBody};
    for (int i = 0; i < kEcuCount; ++i)
        e.kv(toString(static_cast<Ecu>(i)), toString(s.ecu[i]),
             okOr(w.severityOf(WarnId::EcuStatus, i), kGreen));

    // ---- column 3: inverters, autonomous, injected faults
    panel(g, 830, kTop, 440, 165, "INVERTERS");
    Column d{g, 844, kTop + kBody};
    const int ox[6] = {0, 40, 110, 180, 260, 330};
    const char* hdr[6] = {"INV", "TMP", "MOT", "TRQ", "RPM", "STATE"};
    for (int c = 0; c < 6; ++c) g.text(d.x + ox[c], d.y, hdr[c], 18, kDim);
    d.y += kLine;
    for (int i = 0; i < kInverterCount; ++i) {
        const InverterData& iv = s.inverters[i];
        g.text(d.x + ox[0], d.y, kInv[i], 20, kText);
        g.text(d.x + ox[1], d.y, fmt("%.0f", iv.temp_c), 20,
               okOr(w.severityOf(WarnId::InverterTemp, i), kText));
        g.text(d.x + ox[2], d.y, fmt("%.0f", iv.motor_temp_c), 20,
               okOr(w.severityOf(WarnId::MotorTemp, i), kText));
        g.text(d.x + ox[3], d.y, fmt("%.0f", iv.torque_nm), 20, kText);
        g.text(d.x + ox[4], d.y, fmt("%.0f", iv.rpm), 20, kText);
        g.text(d.x + ox[5], d.y, toString(iv.state), 20,
               iv.state == InverterState::Fault ? colorFor(Severity::Critical) : kText);
        d.y += kLine;
    }

    panel(g, 830, kTop + 173, 440, 190, "AUTONOMOUS");
    Column au{g, 844, kTop + 173 + kBody};
    au.kv("ASMS", s.asms_on ? "ON" : "OFF");
    au.kv("AS state", toString(s.as));
    au.kv("EBS", toString(s.ebs), okOr(w.severityOf(WarnId::EbsStatus), kText));
    au.kv("mission", toString(s.mission));
    au.kv("tank", fmt("%.2f bar", s.tank_pressure_bar),
          okOr(w.severityOf(WarnId::LowTankPressure), kText));
    au.kv("brake line", fmt("%.2f bar", s.brake_line_pressure_bar));

    panel(g, 830, kTop + 371, 440, 215, "INJECTED FAULTS");
    Column f{g, 844, kTop + 371 + kBody};
    for (int i = 0; i < kFaultCount; ++i) {
        const Fault fault = faultFromIndex(i);
        const bool on = hasFault(s, fault);
        g.text(f.x, f.y, toString(fault), 20, on ? colorFor(Severity::Critical) : kDim);
        g.text(f.x + 340, f.y, on ? "ON" : "off", 20, on ? colorFor(Severity::Critical) : kDim);
        f.y += kLine;
    }

    drawAlerts(g, Mode::Debugging, s, w, 0, 48);
}

// -------------------------------------------------------------------- MANUAL
void drawManual(Gfx& g, const VehicleState& s, const WarningEngine& w, const Config& cfg) {
    // Speed: the one thing that matters.
    centered(g, kW / 2, 130, fmt("%.0f", s.speed_kmh), 220, kText);
    centered(g, kW / 2, 395, "km/h", 40, kDim);

    // State of charge (left): big value, tall bar, threshold marks read from the config.
    const Color soc_c = okOr(w.severityOf(WarnId::LowSoc), kGreen);
    g.text(40, 150, "SoC", 30, kDim);
    g.text(40, 190, fmt("%.0f%%", s.soc_pct), 90, soc_c);
    bar(g, 40, 310, 300, 56, s.soc_pct / 100.0, soc_c);
    const ThresholdSet& soc_t = cfg.thresholds.soc;
    const std::array<std::pair<double, Severity>, 3> soc_marks = {{
        {soc_t.caution, Severity::Caution},
        {soc_t.warning, Severity::Warning},
        {soc_t.critical, Severity::Critical},
    }};
    for (const auto& m : soc_marks) {
        const int mx = 40 + static_cast<int>(300 * std::clamp(m.first / 100.0, 0.0, 1.0));
        g.fillRect(mx - 1, 304, 3, 68, colorFor(m.second));
    }

    // Key settings (right): small label, big value.
    auto setting = [&g](int y, const char* label, const std::string& value, Color c) {
        g.text(900, y, label, 24, kDim);
        g.text(900, y + 26, value, 48, c);
    };
    setting(150, "POWER",
            fmt("%d/%d", s.power_level, static_cast<int>(cfg.sim.power_levels_max)), kText);
    setting(225, "REGEN",
            fmt("%d/%d", s.regen_level, static_cast<int>(cfg.sim.regen_levels_max)), kText);
    setting(300, "TORQUE VECT.",
            s.tv_level == 0 ? std::string("OFF")
                            : fmt("%d/%d", s.tv_level, static_cast<int>(cfg.sim.tv_levels_max)),
            s.tv_level == 0 ? kDim : kText);
    setting(375, "DRS", s.drs_open ? "OPEN" : "SHUT", s.drs_open ? kGreen : kDim);

    // Drive state.
    const bool drive = s.ts == TsState::Drive;
    centered(g, kW / 2, 450, drive ? "DRIVE" : toString(s.ts), 48, drive ? kGreen : kDim);

    // Pedals + steering.
    g.text(40, 545, "THR", 26, kDim);
    bar(g, 120, 545, 500, 30, s.throttle, kGreen);
    g.text(40, 590, "BRK", 26, kDim);
    bar(g, 120, 590, 500, 30, s.brake, colorFor(Severity::Critical));
    g.text(700, 545, "STEER", 26, kDim);
    const int tx = 700, tw = 480, ty = 600;
    g.fillRect(tx, ty, tw, 8, {35, 38, 48});
    const double f = std::clamp(s.steering_deg / cfg.display.steer_full_deg, -1.0, 1.0);
    g.fillRect(tx + tw / 2 + static_cast<int>(f * tw / 2) - 6, ty - 10, 12, 28, kText);

    drawAlerts(g, Mode::Manual, s, w, 0, 88);
}

// ---------------------------------------------------------------- DRIVERLESS
Color asColor(AsState a, bool blink) {
    switch (a) {
        case AsState::Ready: return {255, 210, 0};
        case AsState::Driving: return blink ? Color{255, 210, 0} : Color{70, 60, 0};
        case AsState::Finished: return {60, 140, 255};
        case AsState::Emergency: return blink ? Color{60, 140, 255} : Color{15, 35, 80};
        default: return kDim;
    }
}

void drawDriverless(Gfx& g, const VehicleState& s, const WarningEngine& w, const Config& cfg) {
    const bool blink = flashOn(s.time_s);

    // Left: AS state, mission, ASMS, GO.
    g.fillRect(40, 130, 580, 190, kPanel);
    g.text(60, 140, "AUTONOMOUS SYSTEM", 26, kDim);
    g.text(60, 180, toString(s.as), 80, asColor(s.as, blink));
    g.text(60, 280, s.asms_on ? "ASMS ON" : "ASMS OFF", 30, s.asms_on ? kGreen : kDim);
    if (s.go_pending) g.text(300, 280, fmt("GO IN %.1f s", s.go_countdown_s), 30, colorFor(Severity::Caution));

    g.fillRect(40, 340, 580, 130, kPanel);
    g.text(60, 350, "MISSION", 26, kDim);
    g.text(60, 385, toString(s.mission), 60, s.mission == Mission::None ? kDim : kText);

    g.text(40, 490, fmt("speed %.0f km/h   target %.0f km/h", s.speed_kmh, s.target_speed_kmh), 26, kDim);

    // Right: EBS, pressures.
    const Severity ebs_sev = w.severityOf(WarnId::EbsStatus);
    Color ebs_c = kDim;
    if (s.ebs == EbsState::Armed) ebs_c = kGreen;
    else if (s.ebs == EbsState::Activated || s.ebs == EbsState::Fault) ebs_c = colorFor(Severity::Critical);
    else if (ebs_sev != Severity::None) ebs_c = colorFor(ebs_sev);
    g.fillRect(680, 130, 560, 100, kPanel);
    g.text(700, 138, "EBS", 26, kDim);
    g.text(700, 172, toString(s.ebs), 52, ebs_c);

    const Color tank_c = okOr(w.severityOf(WarnId::LowTankPressure), kGreen);
    g.text(680, 250, fmt("AIR TANK  %.2f bar", s.tank_pressure_bar), 30, tank_c);
    const int bx = 680, bw = 560;
    bar(g, bx, 290, bw, 30, s.tank_pressure_bar / cfg.display.bar_full_bar, tank_c);
    // Threshold marks, read from the same config the warning engine uses.
    const ThresholdSet& tank_t = cfg.thresholds.tank_pressure;
    const std::array<std::pair<double, Severity>, 3> tank_marks = {{
        {tank_t.caution, Severity::Caution},
        {tank_t.warning, Severity::Warning},
        {tank_t.critical, Severity::Critical},
    }};
    for (const auto& m : tank_marks) {
        const int mx = bx + static_cast<int>(bw * std::clamp(m.first / cfg.display.bar_full_bar, 0.0, 1.0));
        g.fillRect(mx - 1, 284, 3, 42, colorFor(m.second));
        const std::string label = fmt("%g", m.first);
        g.text(mx - g.textWidth(label, 16) / 2, 328, label, 16, colorFor(m.second));
    }

    g.text(680, 350, fmt("BRAKE LINE  %.2f bar", s.brake_line_pressure_bar), 30, kText);
    bar(g, 680, 388, 560, 30, s.brake_line_pressure_bar / cfg.display.bar_full_bar, kHead);

    // Readiness checklist.
    bool ecus_ok = true;
    for (int i = 0; i < kEcuCount; ++i) ecus_ok = ecus_ok && s.ecu[i] == EcuStatus::Ok;
    struct Check {
        const char* name;
        const char* blocker;
        bool ok;
    };
    const std::array<Check, 6> checks = {{
        {"ASMS on", "ASMS off", s.asms_on},
        {"Mission selected", "no mission", s.mission != Mission::None},
        {"TS ready", "TS not ready", s.ts == TsState::Ready || s.ts == TsState::Drive},
        {"EBS armed", "EBS not armed", s.ebs == EbsState::Armed},
        {"All ECUs OK", "ECU fault", ecus_ok},
        {"Tank pressure OK", "low tank pressure",
         w.severityOf(WarnId::LowTankPressure) < Severity::Warning},
    }};
    bool all_ok = true;
    int blocked = 0;
    const Check* first_block = nullptr;
    int y = 440;
    for (const Check& c : checks) {
        g.text(680, y, c.ok ? "[OK]" : "[--]", 24, c.ok ? kGreen : colorFor(Severity::Warning));
        g.text(770, y, c.name, 24, c.ok ? kText : kDim);
        if (!c.ok) {
            all_ok = false;
            ++blocked;
            if (!first_block) first_block = &c;
        }
        y += 28;
    }

    g.fillRect(40, 560, 580, 100, all_ok ? Color{20, 90, 45} : Color{70, 40, 15});
    if (all_ok) {
        centered(g, 330, 572, "SYSTEM READY", 48, kGreen);
        if (s.as == AsState::Ready && !s.go_pending)
            centered(g, 330, 628, "press G to start the run", 24, kText);
    } else {
        centered(g, 330, 572, "NOT READY", 48, colorFor(Severity::Warning));
        std::string why = std::string("blocked by: ") + first_block->blocker;
        if (blocked > 1) why += " +" + std::to_string(blocked - 1) + " more";
        centered(g, 330, 628, why, 24, kText);
    }

    drawAlerts(g, Mode::Driverless, s, w, 0, 88);
}

}  // namespace

void drawPage(Gfx& g, Mode mode, const VehicleState& s, const WarningEngine& w, const Config& cfg) {
    switch (mode) {
        case Mode::Debugging: drawDebugging(g, s, w); break;
        case Mode::Manual: drawManual(g, s, w, cfg); break;
        case Mode::Driverless: drawDriverless(g, s, w, cfg); break;
    }
    drawFooter(g, mode);
}

}  // namespace vds
