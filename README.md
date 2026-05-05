# HID Tester - A Free Joystick Testing App

A lightweight, high-performance tool designed to test generic USB game controllers, joysticks, button boxes, keyboards, and mice across **Windows, Linux, and macOS**. Built for gamers, sim-racing/flight-sim enthusiasts, and hardware developers to analyze axis precision, button inputs, signal curves, and sensor polling rates in real-time. Have a look at this short [Demo](https://github.com/rhunecke/HIDTester/wiki/Demo).

## ✨ Features
* **Cross-Platform:** Native support for Windows, Linux, and macOS (Apple Silicon / ARM64).
* **Dynamic UI Modes:** A context-sensitive toolbar lets you seamlessly swap between Gamepad/Joystick and Keyboard/Mouse testing suites to keep your workspace clean and focused.
* **Keyboard Diagnostics:** Check your hardware limits with a real-time N-Key Rollover visualizer, and inspect the chronological event stream for Key Down/Up actions and raw hardware Scancodes.
* **Mouse Sensor Analysis:** Track your mouse's live Polling Rate (Hz), record peak speeds, and utilize the 2D Sensor Trail Canvas to instantly spot angle snapping, jitter, or spin-outs.
* **Axis Data:** Displays both normalized float values and exact 16-bit SDL-scaled raw integers (-32768 to 32767) to easily detect sensor jitter, drift, or deadzone irregularities.
* **Stick Monitors:** Visualize primary and secondary analog stick movements on a clean 2D coordinate system.
* **Dynamic Axis Curves:** Live time-series graphs for all analog axes with adjustable "Speed" (time window) and "Zoom" controls to easily detect hardware jitter, drift, or deadzone issues.
* **Unified Event Log:** Chronologically tracks button presses and POV hat movements (with precise degrees) and exact millisecond hold-durations.
* **Button & POV Hat Check:** Clear overview of all digital inputs and an 8-way radar-style D-Pad visualizer that also displays degree angles.
* **Adjustable Software Deadzone & Triggers:** Filter out hardware jitter instantly and define axes as unidirectional.
* **Modern UI & High Performance:** Minimal CPU footprint, powered by a clean MVC architecture, SDL2, OpenGL 3, and Dear ImGui.

---

## 🚀 Download & Use (No Installation Required)

If you just want to use the application, you **do not** need to build it from source!
Head over to the [Releases Page](https://github.com/rhunecke/HIDTester/releases) and download the latest `.zip` file for your operating system. 

* **Windows:** Extract and run `HIDTester.exe` (ensure `SDL2.dll` is in the same folder).
* **Linux:** Extract and run the `HIDTester` executable.
* **macOS:** Extract and run the `HIDTester` executable. Dynamic libraries (like SDL2) are now pre-bundled, so no external installations are required via Homebrew.

---

## 🛠️ Setup & Installation for Developers

To keep the repository lightweight, external libraries are not included in the source tree. If you want to build the project yourself, please follow these steps:

### 1. Prerequisites

Depending on your operating system, install the required build tools:

* **Windows:**
  * CMake (Version 3.10 or higher)
  * Visual Studio 2022 (with C++ Desktop development workload)
* **Linux (Ubuntu/Debian):**
  * `sudo apt-get update`
  * `sudo apt-get install cmake g++ libsdl2-dev libgl1-mesa-dev`
* **macOS:**
  * `brew update`
  * `brew install cmake sdl2 dylibbundler`

### 2. Prepare Dependencies (Windows & All Platforms)

Create a `thirdparty` folder in the project root.

#### Dear ImGui (All Platforms)
1. Clone or download [Dear ImGui](https://github.com/ocornut/imgui).
2. Place it at `thirdparty/imgui` (ensure `thirdparty/imgui/backends` exists).

#### SDL2 (Windows Only)
*(Linux and macOS resolve SDL2 globally via their package managers).*
1. Go to the official SDL GitHub releases page: [https://github.com/libsdl-org/SDL/releases](https://github.com/libsdl-org/SDL/releases).
2. Download the latest **SDL2** release (e.g., `SDL2-devel-2.30.x-VC.zip`). *Do not download SDL3.*
3. Extract it to `thirdparty/SDL2`.
4. Ensure the paths `thirdparty/SDL2/include` and `thirdparty/SDL2/lib/x64` exist.

### 3. Build Process

You can use the CMake integration in VS Code or the standard command line:
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release