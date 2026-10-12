#include "core/VehicleState.hpp"

namespace vds {

const char* toString(TsState v) {
    switch (v) {
        case TsState::Off: return "OFF";
        case TsState::Precharge: return "PRECHARGE";
        case TsState::Ready: return "READY";
        case TsState::Drive: return "DRIVE";
    }
    return "?";
}
const char* toString(AsState v) {
    switch (v) {
        case AsState::Off: return "AS OFF";
        case AsState::Ready: return "AS READY";
        case AsState::Driving: return "AS DRIVING";
        case AsState::Finished: return "AS FINISHED";
        case AsState::Emergency: return "AS EMERGENCY";
    }
    return "?";
}
const char* toString(EbsState v) {
    switch (v) {
        case EbsState::Unavailable: return "UNAVAILABLE";
        case EbsState::Armed: return "ARMED";
        case EbsState::Activated: return "ACTIVATED";
        case EbsState::Fault: return "FAULT";
    }
    return "?";
}
const char* toString(EcuStatus v) {
    switch (v) {
        case EcuStatus::Ok: return "OK";
        case EcuStatus::Fault: return "FAULT";
        case EcuStatus::Offline: return "OFFLINE";
    }
    return "?";
}
const char* toString(InverterState v) {
    switch (v) {
        case InverterState::Off: return "OFF";
        case InverterState::Idle: return "IDLE";
        case InverterState::Active: return "ACTIVE";
        case InverterState::Derated: return "DERATED";
        case InverterState::Fault: return "FAULT";
    }
    return "?";
}
const char* toString(Mission v) {
    switch (v) {
        case Mission::None: return "NOT SELECTED";
        case Mission::Acceleration: return "ACCELERATION";
        case Mission::Skidpad: return "SKIDPAD";
        case Mission::Autocross: return "AUTOCROSS";
        case Mission::Trackdrive: return "TRACKDRIVE";
        case Mission::EbsTest: return "EBS TEST";
        case Mission::Inspection: return "INSPECTION";
    }
    return "?";
}
const char* toString(Ecu v) {
    switch (v) {
        case Ecu::Smu: return "SMU";
        case Ecu::Ebs: return "EBS";
        case Ecu::Vcu: return "VCU";
        case Ecu::Bms: return "BMS";
        case Ecu::Asu: return "ASU";
    }
    return "?";
}
const char* toString(Fault v) {
    switch (v) {
        case Fault::None: return "none";
        case Fault::InverterOverTemp: return "Inverter over-temperature";
        case Fault::CriticalSoc: return "Critical SoC drop";
        case Fault::AsFault: return "Autonomous system fault";
        case Fault::EbsFault: return "EBS fault";
        case Fault::TankLeak: return "Air tank leak";
        case Fault::BmsFault: return "BMS fault";
        case Fault::SmuLost: return "SMU offline";
    }
    return "?";
}

}  // namespace vds
