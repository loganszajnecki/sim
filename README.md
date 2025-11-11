# sam_sim — Modular Surface-to-Air Missile Simulation & Visualization

`sam_sim` is a modular C++ framework for real-time and batch simulation of surface-to-air missile engagements, featuring both:
- **Headless execution** for Monte Carlo and statistical analysis.
- **Interactive OpenGL viewer** for live visualization with trails, axes, and textured models.

---

## Architecture Overview

The project is split into three primary namespaces:

| Layer | Namespace | Description |
|:------|:-----------|:------------|
| **Simulation Core** | `sim::` | Physics, integration, and module interfaces (guidance, autopilot, aerodynamics). |
| **Application Glue** | `app::` | Wraps the simulation into run-loops, logging, Monte Carlo, and viewer integration. |
| **Visualization** | `vis::` | Real-time OpenGL rendering system (GLFW + GLAD + ImGui-ready structure). |

All three layers are designed to be **modular and replaceable**. The physics fidelity (3-DoF → 6-DoF), guidance logic, or rendering pipeline can be upgraded independently.

---

## Major Components

### Simulation Core (`src/sim/`)
- **`State`** — Generic container for time + state vector.
- **`Integrator`** — 4th-order Runge–Kutta (`RK4Integrator`).
- **`Missile`** — Aggregates aerodynamic, guidance, and autopilot modules.
- **`IAerodynamics`, `IGuidance`, `IAutopilot`** — Clean interfaces for model-swapping.
- **`SimpleAero`**, **`ProNavGuidance`**, **`PDAutopilot`** — Example 3-DoF models.
- **`JSONFactory`** — Constructs modules from configuration files (e.g. `simple_scenario.json`).

### Application Layer (`src/app/`)
- **`SimulationEngine`** — Drives a single missile–target engagement with RK4 stepping and intercept detection.
- **`LoopController`** — Manages real-time pacing and fixed-dt stepping.
- **`RendererController`** — Manages the viewer and telemetry bus connection.
- **`TelemetryCsvWriter`** — Logs missile/target trajectories to CSV for post-analysis.

### Visualization (`src/vis/`)
- **`Renderer`** — Manages GLFW window, camera, scene draw, and OpenGL resources.
- **`MasterRenderer` / `EntityRenderer`** — Batched, material-based rendering system.
- **`ShaderProgram`, `LineShader`, `EntityShader`** — RAII-safe OpenGL shader wrappers.
- **`Loader`** — Loads VAOs/VBOs/textures (via `stb_image`).
- **`Camera`** — Z-up orbit/pan/dolly camera controller.
- **`TelemetryBus`** — Thread-safe producer–consumer queue for live state streaming.

---

## Building

### Requirements
- **CMake ≥ 3.20**
- **C++17 compiler**
- **OpenGL 3.3+**
- **GLFW**, **GLAD**, and **stb_image** vendored in `third_party/`
- **nlohmann_json** vendored single-header in `third_party/nlohmann/`

### Quick Build

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build both executables
cmake --build build -j

# Run headless mode (fast Monte Carlo)
./build/sam_sim_headless configs/simple_scenario.json

# Run interactive viewer
./build/sam_sim configs/simple_scenario.json
