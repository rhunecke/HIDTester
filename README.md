# HID Tester - A free Joystick Testing App

A lightweight and powerful tool to test game controllers and joysticks on Windows. Designed for gamers, sim-enthusiasts, and developers to analyze axis precision, button inputs, and signal curves in real-time.

## Features
* **Real-time Monitor**: Visualize stick movements on 2D coordinate systems.
* **Axis Curves**: Live time-series graphs for all analog axes to detect jitter or deadzones.
* **Button Check**: Clear overview of all digital inputs.
* **Modern UI**: Clean dashboard based on Dear ImGui with a dark-mode aesthetic.
* **High Performance**: Minimal CPU footprint using SDL2 and OpenGL 3.

## Setup & Installation for Developers

To keep the repository lightweight, external libraries are not included. Please follow these steps to prepare the environment:

### 1. Prerequisites
* **Windows OS**
* **CMake** (Version 3.10 or higher)
* **Visual Studio 2022** (with C++ Desktop development workload)

### 2. Prepare Dependencies
Create a `thirdparty` folder in the project root and add the following:

#### SDL2
1. Go to the official SDL GitHub releases page: [https://github.com/libsdl-org/SDL/releases](https://github.com/libsdl-org/SDL/releases).
2. Scroll down to the latest **SDL2** release (e.g., [SDL 2.32.10](https://github.com/libsdl-org/SDL/tree/release-2.32.10)) - do *not* download SDL3.
3. Download the **SDL2-devel-2.x.x-VC.zip** (Visual C++ development library).
4. Extract it to `thirdparty/SDL2`.
5. Ensure the paths `thirdparty/SDL2/include` and `thirdparty/SDL2/lib/x64` exist.

#### Dear ImGui
1. Download or clone [Dear ImGui](https://github.com/ocornut/imgui).
2. Extract it to `thirdparty/imgui`.
3. Ensure the paths `thirdparty/imgui/backends` exist.

### 3. Build Process
You can use the CMake integration in VS Code or the command line:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Linux Button Limitation
Please note that due to how the Linux Kernel (`evdev`) handles game controllers, there is a default limit of **80 buttons per device** (ID 0 - 79). 

If your controller has more than 80 buttons, the additional inputs will not be recognized by the Linux system and will therefore not show up in this application. This is a platform-specific limitation and not a bug in HID Tester.

**Workaround:** For high-button-count devices like DIY button boxes, it is recommended to configure the hardware as two separate virtual controllers (e.g., 64 buttons each) to ensure full compatibility across all operating systems.

## License
This project is licensed under the GNU GPL v3.0. See the "About" window in the application for more details.