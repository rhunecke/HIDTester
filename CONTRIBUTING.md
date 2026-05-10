# Contributing to HID Tester

Thank you for your interest in contributing to HID Tester! This document describes how to report bugs, suggest features, and submit code contributions.

## Table of Contents

- [Reporting Bugs](#reporting-bugs)
- [Suggesting Features](#suggesting-features)
- [Development Setup](#development-setup)
- [Code Style](#code-style)
- [Submitting a Pull Request](#submitting-a-pull-request)
- [Security Vulnerabilities](#security-vulnerabilities)

---

## Reporting Bugs

Before opening a bug report, please:

1. Check the [existing issues](../../issues) to avoid duplicates.
2. Make sure you are using the [latest release](../../releases/latest).

When filing a bug, please include:

- **OS and version** (e.g., Windows 11, Ubuntu 24.04, macOS 14)
- **HID Tester version** (shown in the About dialog)
- **Steps to reproduce** — as specific as possible
- **Expected behavior** vs. **actual behavior**
- Any relevant screenshots or error output

---

## Suggesting Features

Feature requests are welcome. Open a [GitHub Issue](../../issues/new) and describe:

- **What problem does this solve?** Describe the use case.
- **What should the feature do?** Be as specific as possible.
- **Are there any alternatives you have considered?**

---

## Development Setup

### Prerequisites

See the **Setup & Installation for Developers** section in [README.md](README.md#setup--installation-for-developers) for the full list of required tools and dependencies per platform.

### Building

```bash
# From the project root
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
```

### Running the Application

The built executable is located at `build/HIDTester` (Linux/macOS) or `build/Debug/HIDTester.exe` (Windows).

---

## Code Style

HID Tester is written in C++20. Please follow these conventions when contributing:

- **Formatting**: Match the style of the surrounding code (4-space indentation, K&R brace style).
- **Comments**: Use Doxygen-style `/** ... */` for function documentation; use `//` for inline comments.
- **Safety**:
  - Prefer `snprintf` over `sprintf`; avoid raw `system()` calls.
  - Use `static_cast<>` instead of C-style casts.
  - Avoid signed/unsigned comparison warnings — cast to the appropriate type.
- **Portability**: All code must compile cleanly on Windows (MSVC), Linux (GCC/Clang), and macOS (Clang). Use `#ifdef` guards when platform-specific code is required.
- **Third-party scope**: Do not add new third-party libraries without discussion. The project intentionally keeps its dependency footprint small.

---

## Submitting a Pull Request

1. **Fork** the repository and create a new branch from `main`:
   ```bash
   git checkout -b feature/my-improvement
   ```
2. **Make your changes**. Keep commits focused and atomic.
3. **Test** your changes on at least one platform. If possible, verify on all three (Windows, Linux, macOS).
4. **Push** your branch and open a Pull Request against `main`.
5. **Describe** your PR clearly: what changed, why, and how to test it.

Pull requests that break an existing build or introduce new compiler warnings will be asked to revise before merging.

---

## Security Vulnerabilities

Please **do not** open a public issue for security vulnerabilities. See [SECURITY.md](SECURITY.md) for the responsible disclosure process.
