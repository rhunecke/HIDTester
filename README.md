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
## ⚠️ Platform Specific Limitations & Quirks

When testing high-end sim gear or building your own DIY controllers, please be aware of the following platform and API limitations. These are operating system restrictions, not bugs in HID Tester!

### Windows: 8 Axes & 4 POV Hats Limit
Due to legacy constraints within Microsoft's DirectInput API (which most generic controllers and SDL2 fall back to), Windows is limited to a maximum of **8 analog axes** and **4 POV hats** per device. 
If your controller features more inputs than this, Windows will simply ignore them. 
* **Workaround:** Configure your hardware/firmware to expose itself as multiple virtual controllers (e.g., two devices with 8 axes / 4 POV hats each) to ensure full compatibility.

### Linux: 80 Buttons Limit
Due to how the Linux Kernel (specifically `evdev` and `joydev` legacy mappings) handles game controllers, there is a hard limit of **80 buttons per device** (ID 0 - 79). 
Any inputs exceeding this ID will not be recognized by the Linux system and will therefore not show up in this application. 
* **Workaround:** For high-button-count DIY button boxes, split the firmware configuration into two separate virtual controllers (e.g., 64 buttons each).

### General Note: The "32-Button" Game Engine Limit
While Windows can technically read up to 128 buttons per device (and HID Tester will happily display them!), **many older games and flight sims cap out at 32 buttons**. They store button states in a 32-bit integer, meaning any button from #33 upwards won't register in the game, even if HID Tester sees it perfectly fine. Keep this in mind when mapping your DIY button boxes!

---

## 📄 License
This project is licensed under the GNU GPL v3.0. See the "About" window in the application or the `LICENSE` file for more details.