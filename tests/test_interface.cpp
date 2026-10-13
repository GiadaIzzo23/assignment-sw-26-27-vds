#include <cstddef>
#include <vector>

#include "check.hpp"
#include "core/IVehicleSource.hpp"

using namespace vds;

namespace {

// A stand-in for "the real car": it only fills a VehicleState and records what
// it is asked to do. No simulation is involved.
struct FakeCar : IVehicleSource {
    VehicleState st;
    std::vector<Command> received;
    DriverInput last_input;
    double elapsed = 0;

    void update(double dt) override {
        elapsed += dt;
        st.time_s = elapsed;
    }
    const VehicleState& state() const override { return st; }
    void apply(const Command& c) override { received.push_back(c); }
    void setDriverInput(const DriverInput& in) override { last_input = in; }
};

}  // namespace

TEST(a_source_can_be_driven_through_the_interface_only) {
    FakeCar car;
    car.st.speed_kmh = 42;
    IVehicleSource& source = car;  // the dashboard will only ever see this

    for (int i = 0; i < 60; ++i) source.update(1.0 / 60.0);

    CHECK_NEAR(source.state().time_s, 1.0, 1e-9);
    CHECK_NEAR(source.state().speed_kmh, 42.0, 1e-12);
}

TEST(commands_carry_their_fault_payload) {
    FakeCar car;
    IVehicleSource& source = car;

    source.apply({CommandType::ToggleFault, Fault::TankLeak});
    source.apply({CommandType::DrsToggle});

    CHECK_EQ(car.received.size(), static_cast<std::size_t>(2));
    CHECK(car.received[0].type == CommandType::ToggleFault);
    CHECK(car.received[0].fault == Fault::TankLeak);
    CHECK(car.received[1].type == CommandType::DrsToggle);
    CHECK(car.received[1].fault == Fault::None);  // the payload defaults to "no fault"
}

TEST(driver_input_describes_held_keys) {
    const DriverInput idle;
    CHECK(!idle.accelerate);
    CHECK(!idle.brake);
    CHECK_EQ(idle.steer, 0);

    FakeCar car;
    IVehicleSource& source = car;
    source.setDriverInput({true, false, -1});
    CHECK(car.last_input.accelerate);
    CHECK(!car.last_input.brake);
    CHECK_EQ(car.last_input.steer, -1);
}
