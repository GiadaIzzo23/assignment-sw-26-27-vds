// Plain data describing the car. This struct is the ONLY thing the UI reads.
// A real telemetry/CAN decoder would simply fill the same struct.
#pragma once

#include <array>
#include <cstdint>

namespace vds {

enum class TsState { Off, Precharge, Ready, Drive };               // tractive system
enum class AsState { Off, Ready, Driving, Finished, Emergency };   // autonomous system
enum class EbsState { Unavailable, Armed, Activated, Fault };      // emergency brake
enum class EcuStatus { Ok, Fault, Offline };
enum class InverterState { Off, Idle, Active, Derated, Fault };
enum class Mission { None, Acceleration, Skidpad, Autocross, Trackdrive, EbsTest, Inspection };
enum class Ecu { Smu, Ebs, Vcu, Bms, Asu };

constexpr int kInverterCount = 4;  // FL, FR, RL, RR
constexpr int kEcuCount = 5;
constexpr int kMissionCount = 7;   // including None

// Injectable scenarios, stored as a bit mask in VehicleState::fault_mask.
enum class Fault : std::uint32_t {
    None = 0,
    InverterOverTemp = 1u << 0,
    CriticalSoc = 1u << 1,
    AsFault = 1u << 2,
    EbsFault = 1u << 3,
    TankLeak = 1u << 4,
    BmsFault = 1u << 5,
    SmuLost = 1u << 6,
};
constexpr int kFaultCount = 7;
constexpr Fault faultFromIndex(int i) { return static_cast<Fault>(1u << i); }

struct InverterData {
    double temp_c = 25;
    double motor_temp_c = 25;
    double torque_nm = 0;  // negative = regenerating
    double rpm = 0;
    InverterState state = InverterState::Off;
};

struct VehicleState {
    double time_s = 0;

    // Driving dynamics
    double speed_kmh = 0;
    double throttle = 0;       // 0..1
    double brake = 0;          // 0..1
    double steering_deg = 0;   // + = right
    double distance_m = 0;
    int lap = 0;

    // Accumulator / low voltage
    double soc_pct = 100;
    double pack_voltage = 0;
    double pack_current_a = 0;   // + = discharging
    double pack_power_kw = 0;
    double pack_temp_c = 25;
    double lv_voltage = 12.4;
    TsState ts = TsState::Off;

    std::array<InverterData, kInverterCount> inverters{};
    std::array<EcuStatus, kEcuCount> ecu{};  // indexed by Ecu

    // Driver settings
    int power_level = 3;
    int regen_level = 2;
    int tv_level = 1;      // 0 = off
    bool drs_requested = false;
    bool drs_open = false;

    // Autonomous system
    bool asms_on = false;
    Mission mission = Mission::None;
    AsState as = AsState::Off;
    EbsState ebs = EbsState::Unavailable;
    double tank_pressure_bar = 10;
    double brake_line_pressure_bar = 0;
    double target_speed_kmh = 0;
    double go_countdown_s = 0;
    bool go_pending = false;

    std::uint32_t fault_mask = 0;  // injected scenarios currently active
};

inline bool hasFault(const VehicleState& s, Fault f) {
    return (s.fault_mask & static_cast<std::uint32_t>(f)) != 0;
}
inline EcuStatus ecuStatus(const VehicleState& s, Ecu e) {
    return s.ecu[static_cast<int>(e)];
}

const char* toString(TsState);
const char* toString(AsState);
const char* toString(EbsState);
const char* toString(EcuStatus);
const char* toString(InverterState);
const char* toString(Mission);
const char* toString(Ecu);
const char* toString(Fault);

}  // namespace vds
