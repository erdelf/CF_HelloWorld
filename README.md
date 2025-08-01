# CF_HelloWorld

First foray into Cute Framework.
Mostly as an exercise for C++

---

## HelloWorld – Bouncing Circles Simulation with Cute Framework

![Preview of Bouncing Circles Simulation](Preview.gif)

This project is a high-performance simulation of thousands of bouncing circles, built using [Cute Framework](https://github.com/RandyGaul/cute_framework) in C++. It demonstrates spatial partitioning (quadtree), real-time physics, and rendering with ImGui-based debug overlays.

### Features

- **Bouncing Circles:** Simulates 5000+ circles bouncing and colliding within a window.
- **Spatial Partitioning:** Uses a quadtree for efficient broad-phase collision detection.
- **Physics:** Realistic collision response and wall bouncing.
- **Rendering:** Circles are drawn with color based on position and randomness.
- **Debug Overlay:** ImGui window displays real-time FPS.

### Getting Started

#### Prerequisites

- C++20 compatible compiler
- [CMake](https://cmake.org/) 3.28 or newer
- [Cute Framework](https://github.com/RandyGaul/cute_framework) (automatically fetched via CMake)

#### Building

1. Clone this repository.
2. Run CMake to configure the project:
   ```sh
   cmake -B build_folder -S .
   ```
3. Build the project:
   ```sh
   cmake --build build_folder
   ```
4. Run the executable from the `build_folder/Debug`.

### Code Structure

- `src/main.cpp`: All simulation logic, including object definitions, physics, rendering, and quadtree implementation.
- `CMakeLists.txt`: Build configuration, including automatic fetching of Cute Framework.

### License

This project is for educational and demonstration purposes.
