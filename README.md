# Task 3 — Vehicle Dashboard Simulator
## 1. Introduction
The **Vehicle Dashboard Simulator (VDS)** is a desktop application that simulates the graphical dashboard of the Formula Student vehicle.
The objective of this assignment is to explore the design and implementation of the team's future graphical dashboard before integrating it into the actual vehicle.

The application must be written in **C or C++**, using a graphics library of your choice.
The simulator should reproduce realistic vehicle information and allow the user to interactively change vehicle states, settings and operating conditions.

The assignment is not only about graphical appearance.
The dashboard must communicate the **right information, at the right time, with the right level of importance**.

---
## 2. Main Objective

**Operating Contexts and Information Hierarchy** 
The dashboard must support three distinct operating modes, each tailored to a specific user and purpose. We are not seeking three identical pages that just rearrange the same numbers.

- **Debugging Mode** is intended for workshop engineers when the car is stationary. It requires maximum information density, providing a complete overview of the vehicle state (accumulator voltage, individual motor/inverter data, detailed temperatures, ECU status, etc.).
    
- **Manual Driving Mode** is for the driver on the track. It demands strict visual clarity and must _not_ attempt to display every signal. It should only show essential driving data (speed, State of Charge, key settings) while hiding secondary parameters unless something goes wrong.
    
- **Driverless Mode** is for preparing autonomous runs. Since there is no driver, the focus must shift to mission selection, Autonomous System (AS) states, Emergency Brake System (EBS) status, air tank pressures, and overall system readiness.

**Internal Simulation and Interactivity** The application will not connect to a real vehicle. Instead, you must implement an **internal simulated vehicle state** that tracks dynamic variables such as speed, pedal positions, steering angle, State of Charge (SoC), temperatures, DRS, and active faults. The user must be able to interact with this simulation via keyboard controls to accelerate, brake, change vehicle settings (e.g., torque vectoring, power/regen levels), and switch between dashboard modes.

**Scenarios and Dynamic Warnings** The simulator must allow us to test the dashboard's reaction to various scenarios. Beyond normal driving, users must be able to intentionally trigger critical events, such as high inverter temperatures, critical SoC drops, or autonomous-system faults. When these events occur, the dashboard must react dynamically. A warning cannot simply be a colored number; its visual representation must reflect its severity and be clearly distinguishable from normal operation. Crucially, how a warning is displayed should depend on the active mode (e.g., a rising temperature might just be a number in Debugging mode, but a flashing alert in Manual Driving mode).

**Software Architecture and Configuration** We require a strict architectural separation of concerns. The simulated vehicle state and input logic must be completely decoupled from the graphical rendering loop. The UI should simply consume and display the vehicle state, ensuring that the simulated backend can be easily swapped out for a real vehicle data stream in the future. Additionally, all simulation constants and warning thresholds (e.g., max speed, critical temperature limits) must be centralized in a maintainable way (configuration files or constants), rather than hardcoded into the rendering logic.

**Deliverables, Testing, and Reproducibility** The final delivery must be a highly reproducible repository. Any team engineer should be able to clone the repo, install dependencies, and build the application smoothly. We require a comprehensive README documenting the build process, keyboard controls, and architectural choices. Finally, while we do not expect automated graphical UI testing, you must provide **automated tests** for the underlying logic—specifically vehicle-state transitions, threshold evaluations, and warning detection—proving that the core simulation logic functions independently of the graphics.
