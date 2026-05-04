# HID Tester - A Free Joystick Testing App

A lightweight, high-performance tool designed to test generic USB game controllers, joysticks, and button boxes across **Windows, Linux, and macOS**. Built for gamers, sim-racing/flight-sim enthusiasts, and hardware developers to analyze axis precision, button inputs, and signal curves in real-time. Have a look at this short [Demo](https://github.com/rhunecke/HIDTester/wiki/Demo).

## ✨ Features
* **Cross-Platform:** Native support for Windows, Linux, and macOS (Apple Silicon / ARM64).
* **Axis Data:** Displays both normalized float values and exact 16-bit SDL-scaled raw integers (-32768 to 32767) to easily detect sensor jitter, drift, or deadzone irregularities.
* **Stick Monitors:** Visualize primary and secondary analog stick movements on a clean 2D coordinate system.
* **Axis Curves:** Live time-series graphs for all analog axes that dynamically adapt to your screen size.
* **Button & POV Hat Check:** Clear overview of all digital inputs and an 8-way radar-style D-Pad visualizer.
* **Adjustable Software Deadzone:** Filter out hardware jitter instantly.
* **Trigger & Throttle Checkbox:** Defining an axis as unidirectional.
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
```
*(On Windows, CMake will automatically copy the required `SDL2.dll` to your output directory next to the executable).*

---
## ⚠️ Platform Specific Limitations

### Windows Axis and POV hat Limitation

Please note that due to how that Windows is limited to a maximum of 8 axes per device. 

If your controller has more than 8 axes or more than 4 POV hats, the additional inputs will not be recognized by Windows and will therefore not show up in this application. This is a platform-specific limitation and not a bug in HID Tester.

**Workaround:** For high-axis-count devices or devices with more than 4 POV hats, it is recommended to configure the hardware/firmware as two separate virtual controllers (e.g., 8 axes / 4 POV hats each) to ensure full compatibility across all operating systems.

### Linux Button Limitation

Please note that due to how the Linux Kernel (`evdev`) handles game controllers, there is a default limit of **80 buttons per device** (ID 0 - 79). 

If your controller has more than 80 buttons, the additional inputs will not be recognized by the Linux system and will therefore not show up in this application. This is a platform-specific limitation of the kernel's input system and not a bug in HID Tester.

**Workaround:** For high-button-count devices like DIY button boxes, it is recommended to configure the hardware/firmware as two separate virtual controllers (e.g., 64 buttons each) to ensure full compatibility across all operating systems.

---

## 📄 License
This project is licensed under the GNU GPL v3.0. See the "About" window in the application or the `LICENSE` file for more details.