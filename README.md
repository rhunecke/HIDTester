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
* **Adjustable Software Deadzone & Triggers:** Filter out hardware jitter instantly, define axes as unidirectional, and use the **Zero Out DZ** auto-calibration mode to zero-out sloppy or worn joystick axes per-axis with a single click.  Each axis also exposes its own DZ slider and a **[Zero]** button for fine-grained individual control.
* **Modern UI & High Performance:** Minimal CPU footprint, powered by a clean MVC architecture, SDL3, OpenGL 3, and Dear ImGui.

---

## 🎯 Software Deadzone Calibration (Zero Out DZ)

Old or heavily-used joysticks develop **mechanical slop** — the axes drift at rest and return slightly different non-zero values every frame even when you aren't touching the stick.  HID Tester's deadzone tools measure that drift automatically and set the exact minimum deadzone needed to silence each axis individually.

### How it works

The toolbar contains a **Global DZ** slider (0 – 25%) and helpers:

| Control | Description |
|---|---|
| **Global DZ slider** | Sets the same deadzone for every axis at once. |
| **Zero Out DZ checkbox** | Auto-mode: every frame it reads each non-trigger axis and sets **per-axis** deadzone to that axis's own resting absolute value. The slider is disabled (greyed out) while active. |
| **Copy DZ button** | Copies a per-axis deadzone table to the clipboard. |

Each axis row in the **Live Test** tab also has:

| Control | Description |
|---|---|
| **Per-axis DZ slider** | Fine-grained deadzone for that axis only (overrides Global DZ for that axis). |
| **[Zero] button** | Sets the per-axis DZ to the current resting value — hold the stick still and click. |

The dead-zone region is highlighted in **off-green** inside the progress bar so you can see exactly how wide the deadzone is at a glance.

### Step-by-step calibration

1. Connect your joystick and select it from the **Device** dropdown.
2. Release all sticks and leave them at their natural resting position — do **not** touch them.
3. Either:
   - Tick **Zero Out DZ** — all drifting axes immediately read `0.0000` and each axis gets its own minimum DZ, **or**
   - Click **[Zero]** on individual axis rows to calibrate one axis at a time.
4. Click **Copy DZ** to copy the per-axis deadzone table to the clipboard.

### Example clipboard output

```
# HIDTester Minimum Axis Deadzone Values
# Hold all sticks/axes at rest before capturing.
# Axis  Type      MinDZ%    MinDZ(0-1)  RawValue
  0     Stick      12.50%   0.1250        -4096
  1     Stick       2.38%   0.0238          781
  2     Stick      13.49%   0.1349         4422
  3     Trigger     0.00%   0.0000       -32768
```

Paste these values into your simulator's or game's **axis deadzone** settings.  Most titles accept a percentage (e.g. `13%`) or a normalized float (e.g. `0.1349`).

### Notes

* **Trigger axes** (marked with the `Trg` checkbox) are excluded from Zero Out DZ and the [Zero] button — their resting rail of `-32768` is intentional, not drift.
* The maximum per-axis deadzone is capped at **25%**.  If drift exceeds that, the hardware may need servicing.
* Unchecking **Zero Out DZ** restores manual control and keeps the last computed values in the per-axis sliders.

---

## 🚀 Download & Use (No Installation Required)

If you just want to use the application, you **do not** need to build it from source!
Head over to the [Releases Page](https://github.com/rhunecke/HIDTester/releases) and download the latest `.zip` file for your operating system. 

* **Windows:** Extract and run `HIDTester.exe` (ensure `SDL3.dll` is in the same folder).
* **Linux:** Extract and run the `HIDTester` executable.
* **macOS:** Extract and run the `HIDTester` executable. Dynamic libraries (like SDL3) are now pre-bundled, so no external installations are required via Homebrew.

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
  * `sudo apt-get install cmake g++ libgl1-mesa-dev`
  * Install SDL3 from your distro package manager if available, or build the official SDL3 release locally and point CMake at that install prefix.
* **macOS:**
  * `brew update`
  * `brew install cmake sdl3`

### 2. Prepare Dependencies (Windows & All Platforms)

Create a `thirdparty` folder in the project root.

#### Dear ImGui (All Platforms)
1. Clone or download [Dear ImGui](https://github.com/ocornut/imgui).
2. Place it at `thirdparty/imgui` (ensure `thirdparty/imgui/backends` exists).

#### SDL3 (Windows Only)
*(Linux and macOS should resolve SDL3 globally via their package managers or a local SDL3 install prefix).*
1. Go to the official SDL GitHub releases page: [https://github.com/libsdl-org/SDL/releases](https://github.com/libsdl-org/SDL/releases).
2. Download the latest **SDL3** Visual C++ development release (e.g., `SDL3-devel-3.4.x-VC.zip`).
3. Extract it to `thirdparty/SDL3`.
4. Ensure the paths `thirdparty/SDL3/include` and `thirdparty/SDL3/lib/x64` exist.

### 3. Build Process

You can use the CMake integration in VS Code or the standard command line:
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

See [MIGRATION.md](MIGRATION.md) for the SDL3 migration note.

### 4. Running the Unit Tests

The unit tests cover the core deadzone math and require **no SDL dependency**.

```bash
mkdir build
cd build
cmake -DBUILD_TESTS=ON ..
cmake --build .
ctest --output-on-failure
```

Expected output:

```
=== HIDTester Deadzone Unit Tests ===
...
Results: 29 passed, 0 failed
```

---

## 🤝 Contributing

Contributions, bug reports, and feature requests are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

## 🔒 Security

If you discover a security vulnerability, please follow the responsible disclosure process described in [SECURITY.md](SECURITY.md). Do not open a public issue.

## 📄 License

This project is licensed under the [GNU GPL v3.0](LICENSE).
