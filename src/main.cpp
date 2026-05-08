#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "JoystickHandler.h"
#include <vector>
#include <string>
#include <iostream>
#include <deque>
#include <algorithm>
#include "version.h"
#include <cstdlib> // Required for system() command on Linux/macOS
#include <chrono>  // Required for Macro Event Log timing
#include <map>     // Required for tracking button states

/**
 * Helper to convert SDL_HAT values to human-readable strings (8-way support)
 */
const char* GetHatDirString(uint8_t value) {
    switch (value) {
        case SDL_HAT_CENTERED:  return "Centered";
        case SDL_HAT_UP:        return "Up";
        case SDL_HAT_RIGHT:     return "Right";
        case SDL_HAT_DOWN:      return "Down";
        case SDL_HAT_LEFT:      return "Left";
        case SDL_HAT_RIGHTUP:   return "Up-Right";
        case SDL_HAT_RIGHTDOWN: return "Down-Right";
        case SDL_HAT_LEFTUP:    return "Up-Left";
        case SDL_HAT_LEFTDOWN:  return "Down-Left";
        default:                return "Unknown";
    }
}
/**
 * Converts SDL_HAT bitmasks into standard game engine degrees (0-360).
 * 0 degrees is strictly UP (North), rotating clockwise.
 */
int GetHatDegree(uint8_t value) {
    switch (value) {
        case SDL_HAT_UP:        return 0;
        case SDL_HAT_RIGHTUP:   return 45;
        case SDL_HAT_RIGHT:     return 90;
        case SDL_HAT_RIGHTDOWN: return 135;
        case SDL_HAT_DOWN:      return 180;
        case SDL_HAT_LEFTDOWN:  return 225;
        case SDL_HAT_LEFT:      return 270;
        case SDL_HAT_LEFTUP:    return 315;
        default:                return -1; // Centered or invalid
    }
}

// --- Data structure for the unified macro event log (Joysticks) ---
struct InputEvent {
    std::string eventName; // e.g., "Button 02" or "Hat 00 (Up-Right, 45°)"
    bool isDown;           // Tracks if the event is a press or release
    double durationMs;     // 0.0 for press events, actual duration for release events
};

/**
 * Visualizes a 2D Joystick coordinate system
 */
void DrawJoystickMonitor(const char* label, float axisX, float axisY, float size = 180.0f) {
    ImGui::BeginGroup();
    ImGui::Text("%s", label);
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 sz = ImVec2(size, size);
    ImVec2 p1 = ImVec2(p0.x + sz.x, p0.y + sz.y);
    ImVec2 center = ImVec2(p0.x + sz.x * 0.5f, p0.y + sz.y * 0.5f);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(p0, p1, IM_COL32(25, 25, 25, 255));
    draw_list->AddRect(p0, p1, IM_COL32(80, 80, 80, 255), 0.0f, 0, 2.0f);
    draw_list->AddLine(ImVec2(p0.x, center.y), ImVec2(p1.x, center.y), IM_COL32(60, 60, 60, 255));
    draw_list->AddLine(ImVec2(center.x, p0.y), ImVec2(center.x, p1.y), IM_COL32(60, 60, 60, 255));

    #ifdef __APPLE__
    // macOS Apple Frameworks interpret "Up" as positive.
    // Since ImGui draws the Y-axis from top to bottom (0 is top),
    // we must invert the visual signal for Mac users,
    // WITHOUT altering the real raw data in the progress bars.
    axisY = -axisY;

    // Note: If the X-axis is also mirrored on Mac during your tests
    // (Left/Right flipped), you can uncomment the following line:
    // axisX = -axisX;
    #endif

    ImVec2 dot_pos = ImVec2(center.x + (axisX * sz.x * 0.5f), center.y + (axisY * sz.y * 0.5f));
    draw_list->AddCircleFilled(dot_pos, 6.0f, IM_COL32(0, 0, 0, 150));
    draw_list->AddCircleFilled(dot_pos, 5.0f, IM_COL32(0, 255, 100, 255));

    ImGui::Dummy(sz);
    ImGui::EndGroup();
}

// Visualizes an 8-way POV Hat (D-Pad) with a radar-style crosshair
void DrawHatVisualizer(const char* label, uint8_t hatState, float size = 80.0f) {
    ImGui::BeginGroup();

    // Force a strict minimum width for this entire block.
    // This stops the UI from shifting horizontally when the text length changes.
    float blockWidth = 160.0f;

    // Center the label text
    float labelWidth = ImGui::CalcTextSize(label).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (blockWidth - labelWidth) * 0.5f);
    ImGui::Text("%s", label);

    // Center the radar drawing area
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (blockWidth - size) * 0.5f);
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 sz = ImVec2(size, size);
    ImVec2 center = ImVec2(p0.x + sz.x * 0.5f, p0.y + sz.y * 0.5f);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddCircleFilled(center, size * 0.5f, IM_COL32(25, 25, 25, 255));
    draw_list->AddCircle(center, size * 0.5f, IM_COL32(80, 80, 80, 255), 0, 2.0f);
    draw_list->AddLine(ImVec2(center.x - size * 0.5f, center.y), ImVec2(center.x + size * 0.5f, center.y), IM_COL32(60, 60, 60, 255));
    draw_list->AddLine(ImVec2(center.x, center.y - size * 0.5f), ImVec2(center.x, center.y + size * 0.5f), IM_COL32(60, 60, 60, 255));

    float diag = size * 0.35f;
    draw_list->AddLine(ImVec2(center.x - diag, center.y - diag), ImVec2(center.x + diag, center.y + diag), IM_COL32(45, 45, 45, 255));
    draw_list->AddLine(ImVec2(center.x + diag, center.y - diag), ImVec2(center.x - diag, center.y + diag), IM_COL32(45, 45, 45, 255));

    float dx = 0.0f, dy = 0.0f;
    if (hatState & SDL_HAT_UP)    dy = -1.0f;
    if (hatState & SDL_HAT_DOWN)  dy =  1.0f;
    if (hatState & SDL_HAT_LEFT)  dx = -1.0f;
    if (hatState & SDL_HAT_RIGHT) dx =  1.0f;

    if (dx != 0.0f && dy != 0.0f) { dx *= 0.7071f; dy *= 0.7071f; }

    ImVec2 dot_pos = ImVec2(center.x + (dx * size * 0.4f), center.y + (dy * size * 0.4f));
    draw_list->AddCircleFilled(dot_pos, 6.0f, IM_COL32(0, 0, 0, 150));
    draw_list->AddCircleFilled(dot_pos, 5.0f, IM_COL32(0, 255, 100, 255));

    ImGui::Dummy(sz); // Reserves vertical space for the radar drawing

    // Format the status string
    std::string statusText;
    int degree = GetHatDegree(hatState);
    if (degree >= 0) {
        statusText = std::string(GetHatDirString(hatState)) + " (" + std::to_string(degree) + "°)";
    } else {
        statusText = GetHatDirString(hatState);
    }

    // Center the status text
    float textWidth = ImGui::CalcTextSize(statusText.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (blockWidth - textWidth) * 0.5f);
    ImGui::TextDisabled("%s", statusText.c_str());

    // Invisible dummy to definitively lock the group's minimum bounding box width
    ImGui::Dummy(ImVec2(blockWidth, 0.0f));

    ImGui::EndGroup();
}

/**
 * Opens a web browser with the specified URL across different operating systems.
 */
void OpenWebpage(const char* url) {
    #if defined(_WIN32)
    // Windows specific URL opening
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    #elif defined(__APPLE__)
    // macOS specific URL opening
    std::string command = "open ";
    command += url;
    system(command.c_str());
    #elif defined(__linux__)
    // Linux specific URL opening
    std::string command = "xdg-open ";
    command += url;
    system(command.c_str());
    #else
    std::cout << "URL opening not supported on this platform." << std::endl;
    #endif
}

/**
 * Visualizes Analog Axes with ProgressBars.
 * Displays both the smoothed logical value and the raw 16-bit hardware data.
 * Allows toggling between Bidirectional (Stick) and Unidirectional (Trigger) modes.
 */
void DrawAnalogAxes(JoystickHandler& joyHandler) {
    const JoystickState& state = joyHandler.getState();
    int numAxes = (int)state.sdlAxes.size();

    if (numAxes > 0) {
        ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Analog Axes (Logical vs. Hardware)");

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Use the 'Trg' checkbox if an axis rests at its minimum value (e.g. Xbox RT/LT).\n"
                "The progress bar shows the smoothed logical input (respecting your deadzone)."
            );
        }

        ImGui::Separator();
        ImGui::Spacing();

        for (int i = 0; i < numAxes; i++) {
            // ImGui needs a unique ID for repeating elements in a loop to track clicks properly
            ImGui::PushID(i);

            ImGui::Text("Axis %d:", i);
            ImGui::SameLine(60);

            // Toggle Switch for Trigger Mode
            bool isTrigger = joyHandler.isAxisTrigger(i);
            if (ImGui::Checkbox("Trg", &isTrigger)) {
                joyHandler.setAxisTriggerMode(i, isTrigger);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Check if this is a Trigger/Throttle (rests at minimum).");
            }

            ImGui::SameLine(115);

            // Raw hardware value (will jitter naturally)
            int16_t rawSdlValue = state.sdlAxes[i];

            // Smoothed logical value with deadzone applied
            float smoothedFloat = state.axes[i];

            float barFillLength = 0.0f;
            ImVec4 barColor = ImVec4(0.2f, 0.6f, 0.2f, 1.0f); // Default inactive color

            if (isTrigger) {
                // Visual logic for Triggers (Starts at 0.0, goes to 1.0)
                barFillLength = smoothedFloat;
                // Highlight if actively pressed
                if (smoothedFloat > 0.01f) {
                    barColor = ImVec4(0.0f, 0.8f, 0.3f, 1.0f);
                }
            } else {
                // Visual logic for Sticks (Starts at center, maps -1.0 -> 1.0 to 0.0 -> 1.0)
                barFillLength = (smoothedFloat + 1.0f) * 0.5f;
                // Highlight if pushed out of the center deadzone
                if (std::abs(smoothedFloat) > 0.05f) {
                    barColor = ImVec4(0.0f, 0.8f, 0.3f, 1.0f);
                }
            }

            // Draw the progress bar with the smoothed float text overlay
            char overlayText[32];
            snprintf(overlayText, sizeof(overlayText), "%.4f", smoothedFloat);

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
            ImGui::ProgressBar(barFillLength, ImVec2(-60, 16), overlayText);
            ImGui::PopStyleColor();

            // Display the exact raw integer value provided by the API
            ImGui::SameLine();
            ImGui::TextDisabled("%6d", rawSdlValue);

            ImGui::PopID(); // End of unique ID context
        }
    }
}

// Global Application Mode
// Defines which testing suite is currently visible to the user.
enum class AppMode {
    Joystick,
    KeyboardMouse
};

// --- Data structure for the macro event log ---
struct ButtonEvent {
    int buttonIndex;
    double durationMs;
};

// --- Keyboard Tracking Data ---
struct KeyboardEvent {
    std::string keyName;
    SDL_Scancode scancode;
    bool isDown;
    double durationMs; // 0.0 for press events, actual duration for release events
};

// --- Mouse Tracking Data ---
struct MouseState {
    int x, y;
    int deltaX, deltaY;
    int wheelDelta;
    bool buttons[5]; // Left, Middle, Right, X1, X2
    std::deque<ImVec2> movementTrail; // For drawing the 2D Scatter plot

    // For Polling Rate Calculation
    uint32_t lastEventTime = 0;
    float currentHz = 0.0f;
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) return -1;

    const char* glsl_version = "#version 130";

    // --- Platform-specific OpenGL setup ---

    #ifdef __APPLE__

    // macOS (Arm) REQUIRES Core Profile + newer GLSL

    glsl_version = "#version 150";



    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);



    // macOS requires Forward Compatibility

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,

                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | SDL_GL_CONTEXT_DEBUG_FLAG

    );

    #else

    // Windows / Linux

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    #endif

    SDL_Window* window = SDL_CreateWindow("HID Tester - A Free Joystick Testing App",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          1280, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    // --- ICON LOADING ---
    #ifdef _WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), "IDI_ICON1");
        if (hIcon) {
            HWND hwnd = wmInfo.info.win.window;
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
    }
    #endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.WindowBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8, 8);

    ImGui::StyleColorsDark();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_Header]   = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // --- DISABLE VSYNC FOR HARDWARE POLLING ---
    // explicitly turn off VSync (Swap Interval 0).
    // This allows the main while() loop to run as fast as the CPU allows (uncapped FPS).
    // This is STRICTLY REQUIRED so SDL_PollEvent can catch 1000Hz+ mouse interrupts
    // instantly without the OS coalescing/batching them during VSync sleep periods.
    SDL_GL_SetSwapInterval(0);

    JoystickHandler joyHandler;

    // --- Force explicit selection ---
    // Start with -1 to indicate no device is currently selected.
    static int selectedDevice = -1;

    static int axisX_idx = 0, axisY_idx = 1, axisX2_idx = 2, axisY2_idx = 3;
    static bool showVisualizer = true;
    static bool show_about_window = false;
    bool deviceOpened = false;

    std::vector<std::deque<float>> axisHistory;

    // Debug Stress Test
    static bool debug_StressTestUI = false; // Toggles the UI rendering stress test

    // --- State variables for Axis Curves and Macro Log ---
    static int maxSamples = 500;      // Controls speed / time window
    static float zoomLevel = 1.0f;    // Controls Y-axis zoom

    std::vector<InputEvent> eventLog;
    std::map<int, std::chrono::steady_clock::time_point> buttonPressTimestamps;

    // Maps hat index to a pair of <current_direction, start_time>
    std::map<int, std::pair<uint8_t, std::chrono::steady_clock::time_point>> hatStateTimestamps;

    // State variables for Keyboard and Mouse
    std::vector<KeyboardEvent> kbEventLog;
    std::vector<SDL_Scancode> keysCurrentlyHeld;

    // Tracks the exact time a key was pressed to calculate its duration
    std::map<SDL_Scancode, std::chrono::steady_clock::time_point> keyPressTimestamps;

    MouseState mouseState = {};

    bool done = false;

    AppMode currentMode = AppMode::Joystick;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;

            // Keyboard Event Tracking
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                // Ignore key repeats (holding down a key) for clean analysis
                if (event.key.repeat == 0) {
                    bool isDown = (event.type == SDL_KEYDOWN);
                    SDL_Scancode scancode = event.key.keysym.scancode;
                    std::string keyName = SDL_GetKeyName(event.key.keysym.sym);

                    double duration = 0.0;

                    if (isDown) {
                        // Record press time
                        keyPressTimestamps[scancode] = std::chrono::steady_clock::now();
                    } else {
                        // Calculate duration on release
                        if (keyPressTimestamps.find(scancode) != keyPressTimestamps.end()) {
                            auto pressTime = keyPressTimestamps[scancode];
                            auto releaseTime = std::chrono::steady_clock::now();
                            std::chrono::duration<double, std::milli> elapsed = releaseTime - pressTime;
                            duration = elapsed.count();
                            keyPressTimestamps.erase(scancode);
                        }
                    }

                    // Add to event log with matching structure
                    kbEventLog.push_back({keyName, scancode, isDown, duration});
                    if (kbEventLog.size() > 100) kbEventLog.erase(kbEventLog.begin());

                    // Track currently held keys for N-Key Rollover testing
                    if (isDown) {
                        keysCurrentlyHeld.push_back(scancode);
                    } else {
                        keysCurrentlyHeld.erase(std::remove(keysCurrentlyHeld.begin(), keysCurrentlyHeld.end(), scancode), keysCurrentlyHeld.end());
                    }
                }
            }

            // Mouse Event Tracking
            if (event.type == SDL_MOUSEMOTION) {
                mouseState.x = event.motion.x;
                mouseState.y = event.motion.y;
                mouseState.deltaX = event.motion.xrel;
                mouseState.deltaY = event.motion.yrel;

                // Calculate Polling Rate (Hz)
                uint32_t currentTime = SDL_GetTicks();
                uint32_t timeDiff = currentTime - mouseState.lastEventTime;
                if (timeDiff > 0) {
                    mouseState.currentHz = 1000.0f / (float)timeDiff;
                }
                mouseState.lastEventTime = currentTime;

                // Store trail points for the visualizer
                mouseState.movementTrail.push_back(ImVec2((float)mouseState.x, (float)mouseState.y));
                if (mouseState.movementTrail.size() > 200) mouseState.movementTrail.pop_front();
            }
            if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                bool isDown = (event.type == SDL_MOUSEBUTTONDOWN);
                if (event.button.button == SDL_BUTTON_LEFT)   mouseState.buttons[0] = isDown;
                if (event.button.button == SDL_BUTTON_MIDDLE) mouseState.buttons[1] = isDown;
                if (event.button.button == SDL_BUTTON_RIGHT)  mouseState.buttons[2] = isDown;
                if (event.button.button == SDL_BUTTON_X1)     mouseState.buttons[3] = isDown;
                if (event.button.button == SDL_BUTTON_X2)     mouseState.buttons[4] = isDown;
            }
            if (event.type == SDL_MOUSEWHEEL) {
                mouseState.wheelDelta = event.wheel.y; // Positive is up, negative is down
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // --- Data Update Layer (Runs before UI rendering for continuous tracking) ---
        if (deviceOpened) {
            joyHandler.update();
            const auto& state = joyHandler.getState();

            // Process Event Log for Buttons
            for (int i = 0; i < (int)state.buttons.size(); i++) {
                bool isDown = state.buttons[i];
                bool wasDown = (buttonPressTimestamps.find(i) != buttonPressTimestamps.end());

                char buffer[32];
                snprintf(buffer, sizeof(buffer), "Button %02d", i);

                if (isDown && !wasDown) {
                    // Button Pressed
                    buttonPressTimestamps[i] = std::chrono::steady_clock::now();
                    eventLog.push_back({std::string(buffer), true, 0.0});
                    if (eventLog.size() > 100) eventLog.erase(eventLog.begin());

                } else if (!isDown && wasDown) {
                    // Button Released
                    auto pressTime = buttonPressTimestamps[i];
                    auto releaseTime = std::chrono::steady_clock::now();
                    std::chrono::duration<double, std::milli> elapsed = releaseTime - pressTime;

                    eventLog.push_back({std::string(buffer), false, elapsed.count()});
                    if (eventLog.size() > 100) eventLog.erase(eventLog.begin());
                    buttonPressTimestamps.erase(i);
                }
            }

            // Process Event Log for POV Hats
            for (int i = 0; i < (int)state.hats.size(); i++) {
                uint8_t currentHat = state.hats[i];

                bool wasActive = (hatStateTimestamps.find(i) != hatStateTimestamps.end());
                uint8_t previousHat = wasActive ? hatStateTimestamps[i].first : SDL_HAT_CENTERED;

                if (currentHat != previousHat) {

                    // Release previous direction
                    if (wasActive && previousHat != SDL_HAT_CENTERED) {
                        auto pressTime = hatStateTimestamps[i].second;
                        auto releaseTime = std::chrono::steady_clock::now();
                        std::chrono::duration<double, std::milli> elapsed = releaseTime - pressTime;

                        int degree = GetHatDegree(previousHat);
                        std::string dirName = GetHatDirString(previousHat);

                        char buffer[64];
                        snprintf(buffer, sizeof(buffer), "Hat %02d (%s, %d°)", i, dirName.c_str(), degree);

                        eventLog.push_back({std::string(buffer), false, elapsed.count()});
                        if (eventLog.size() > 100) eventLog.erase(eventLog.begin());

                        hatStateTimestamps.erase(i);
                    }

                    // Press new direction
                    if (currentHat != SDL_HAT_CENTERED) {
                        hatStateTimestamps[i] = {currentHat, std::chrono::steady_clock::now()};

                        int degree = GetHatDegree(currentHat);
                        std::string dirName = GetHatDirString(currentHat);

                        char buffer[64];
                        snprintf(buffer, sizeof(buffer), "Hat %02d (%s, %d°)", i, dirName.c_str(), degree);

                        eventLog.push_back({std::string(buffer), true, 0.0});
                        if (eventLog.size() > 100) eventLog.erase(eventLog.begin());
                    }
                }
            }
            // --- FIXED TIMESTEP FOR AXIS CURVES ---
            // Update the visual history graphs approximately 60 times per second (every ~16 ms),
            // completely independent of the application's uncapped frame rate.
            static Uint32 lastCurveUpdate = SDL_GetTicks();
            Uint32 currentTime = SDL_GetTicks();

            if (currentTime - lastCurveUpdate >= 16) {
                // Process Axis History
                if (axisHistory.size() != state.axes.size()) axisHistory.resize(state.axes.size());
                for (int i = 0; i < (int)state.axes.size(); i++) {
                    axisHistory[i].push_back(state.axes[i]);
                    while (axisHistory[i].size() > static_cast<size_t>(maxSamples)) {
                        axisHistory[i].pop_front();
                    }
                }
                lastCurveUpdate = currentTime; // Reset the timer
            }
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("MainViewport", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        // --- TOOLBAR ---
        ImGui::BeginChild("Toolbar", ImVec2(0, 50), true, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {

            // --- Compact Mode Switch (Toggle Button) ---
            ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Mode:");
            ImGui::SameLine(0, 5.0f);

            // A compact button that toggles the mode when clicked
            const char* modeLabel = (currentMode == AppMode::Joystick) ? " Joystick " : " KB / Mouse ";
            if (ImGui::Button(modeLabel)) {
                currentMode = (currentMode == AppMode::Joystick) ? AppMode::KeyboardMouse : AppMode::Joystick;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to toggle between Joystick and Keyboard/Mouse testing suites.");
            }

            ImGui::SameLine(0, 15.0f);
            ImGui::TextDisabled("|");
            ImGui::SameLine(0, 15.0f);

            // --- Context Sensitive Toolbar Controls ---
            if (currentMode == AppMode::Joystick) {
                ImGui::Text("Device:");
                ImGui::SetNextItemWidth(300);
                int nJoysticks = SDL_NumJoysticks();

                // --- Dynamic Dropdown Label ---
                std::string currentNameStr = "Select a device...";
                if (nJoysticks == 0) {
                    currentNameStr = "No Device Detected";
                } else if (selectedDevice >= 0 && selectedDevice < nJoysticks) {
                    // Put ID in front of names
                    currentNameStr = "[" + std::to_string(selectedDevice) + "] " + SDL_JoystickNameForIndex(selectedDevice);
                }

                if (ImGui::BeginCombo("##DeviceSelector", currentNameStr.c_str())) {
                    for (int i = 0; i < nJoysticks; i++) {
                        bool isSelected = (selectedDevice == i);
                        std::string deviceName = SDL_JoystickNameForIndex(i);
                        std::string visibleLabel = "[" + std::to_string(i) + "] " + deviceName;

                        if (ImGui::Selectable(visibleLabel.c_str(), isSelected)) {
                            selectedDevice = i;
                            deviceOpened = joyHandler.open(selectedDevice);

                            axisHistory.clear();

                            // --- CRASH FIX ---
                            // Immediately resize the history buffer to match the new device's axis count.
                            if (deviceOpened) {
                                axisHistory.resize(joyHandler.getState().axes.size());
                            }

                            // Clear all macro tracking states on device switch
                            eventLog.clear();
                            buttonPressTimestamps.clear();
                            hatStateTimestamps.clear();
                        }

                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine(0.0f, 30.0f);
                ImGui::Checkbox("Show Stick Monitors", &showVisualizer);

                // --- Float/Raw Deadzone Slider ---
                ImGui::SameLine(0.0f, 30.0f);
                static float deadzoneFloat = 0.0f; // Range: 0.0 to 0.25 (0% to 25%)

                ImGui::SetNextItemWidth(120);

                // Using ImGuiSliderFlags_AlwaysClamp ensures that manually typed values (via CTRL+Click)
                // are strictly clamped between our min (0.0f) and max (0.25f) limits.
                if (ImGui::SliderFloat("Deadzone", &deadzoneFloat, 0.0f, 0.25f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                    // Convert the float representation back to the 16-bit hardware scale (0 to 32767)
                    int16_t rawLimit = static_cast<int16_t>(deadzoneFloat * 32767.0f);
                    joyHandler.setDeadzone(rawLimit);
                }

                // Combined Tooltip with clear UI instructions for manual input
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Filters out small axis movements near the center to prevent jitter.\n"
                    "Shown as normalized float (e.g., 0.100 = 10%%) and raw 16-bit integer.\n"
                    "[Tip: CTRL+Click on the slider to type an exact value]");
                }

                // Display the exact raw hardware value next to it for diagnostics
                ImGui::SameLine();
                ImGui::TextDisabled("(Raw: %d)", static_cast<int>(deadzoneFloat * 32767.0f));

            } else if (currentMode == AppMode::KeyboardMouse) {
                // Render Keyboard/Mouse specific controls
                ImGui::TextDisabled("Listening to global input events...");
            }

            // Debug Menu & About Button
            ImGui::SameLine(ImGui::GetWindowWidth() - 110); // Default position of "About"

            // Renders Debug menu when using Debug build
            #ifndef NDEBUG
            ImGui::SameLine(ImGui::GetWindowWidth() - 200);
            if (ImGui::BeginMenu(" 🛠 Debug ")) {
                ImGui::MenuItem("Enable UI Stress Test", NULL, &debug_StressTestUI);
                ImGui::EndMenu();
            }
            ImGui::SameLine();
            #endif

            if (ImGui::Button("About (?)", ImVec2(90, 0))) show_about_window = true;

            ImGui::EndMenuBar();
        }
        ImGui::EndChild();

        // --- ABOUT MODAL WINDOW ---

        // Trigger the popup opening when the flag is set
        if (show_about_window) {
            ImGui::OpenPopup("About HID Tester");
        }

        if (ImGui::BeginPopupModal("About HID Tester", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize)) {

            // Header: App Title and Version (using Title Case)
            ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "HID Tester - A Free Joystick Testing App");
            ImGui::Text("Version: %s", VERSION_STRING);
            ImGui::Separator();

            // Developer Branding with Interactive Link
            ImGui::Text("Created by: ");
            ImGui::SameLine(0, 5.0f);
            ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "rhunecke");

            // Logic for the developer link (Tooltip and Cross-Platform Opening)
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGui::SetTooltip("www.roberthunecke.com");
            }
            if (ImGui::IsItemClicked()) {
                OpenWebpage("https://www.roberthunecke.com");
            }

            ImGui::Spacing();

            // License Information block
            ImGui::Text("License: GNU GPL v3.0");
            ImGui::TextWrapped("This program is free software: you can redistribute it and/or modify it.");
            ImGui::Spacing();

            // Footer Buttons: Repository Link and Close
            if (ImGui::Button("GitHub Project", ImVec2(120, 0))) {
                OpenWebpage("https://github.com/rhunecke/HIDTester");
            }

            ImGui::SameLine();

            if (ImGui::Button("Close", ImVec2(120, 0))) {
                show_about_window = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // --- CONTENT ---
        if (ImGui::BeginTabBar("MainTabs")) {

            // JOYSTICK TABS
            if (currentMode == AppMode::Joystick) {
                if (ImGui::BeginTabItem("Live Test")) {
                    if (deviceOpened) {
                        JoystickState displayState = joyHandler.getState();

                        if (debug_StressTestUI) {
                            // Maximize Axes (8 is the standard DirectInput Limit)
                            displayState.sdlAxes.assign(8, 16384);
                            displayState.axisIsTrigger.assign(8, false);

                            // Maximize Buttons (128 is the technical limit for many APIs)
                            displayState.buttons.assign(128, false);
                            displayState.buttons[15] = true;  // Fake a few pressed buttons
                            displayState.buttons[127] = true; // Test the absolute last button

                            // --- ANIMATED HATS (Time-based cycling) ---
                            // Array of all 9 possible hat states
                            const uint8_t hat_cycle[] = {
                                SDL_HAT_CENTERED, SDL_HAT_UP, SDL_HAT_RIGHTUP, SDL_HAT_RIGHT,
                                SDL_HAT_RIGHTDOWN, SDL_HAT_DOWN, SDL_HAT_LEFTDOWN, SDL_HAT_LEFT, SDL_HAT_LEFTUP
                            };

                            // Use the time (in seconds) to smoothly cycle through the array
                            double t = ImGui::GetTime();

                            // Maximize Hats (4 is the standard limit)
                            displayState.hats.assign(4, SDL_HAT_CENTERED);

                            // Give each hat a slightly different speed and starting offset
                            displayState.hats[0] = hat_cycle[(int)(t * 2.0) % 9];       // Changes every 0.5 seconds
                            displayState.hats[1] = hat_cycle[(int)(t * 1.5 + 3) % 9];   // Slightly slower, different angle
                            displayState.hats[2] = hat_cycle[(int)(t * 4.0 + 6) % 9];   // Very fast!
                            displayState.hats[3] = hat_cycle[(int)(t * 0.8 + 8) % 9];   // Very slow, almost static
                        }



                        if (showVisualizer && displayState.axes.size() >= 2) {
                            ImGui::BeginChild("VisualizerArea", ImVec2(240, 0), true);
                            ImGui::Text("Joystick Mapping");
                            ImGui::Separator();

                            auto drawCombo = [&](const char* id, int* idx) {
                                ImGui::SetNextItemWidth(90);
                                if (ImGui::BeginCombo(id, ("Axis " + std::to_string(*idx)).c_str())) {
                                    for (int i = 0; i < (int)displayState.axes.size(); i++) {
                                        if (ImGui::Selectable(("Axis " + std::to_string(i)).c_str(), *idx == i)) *idx = i;
                                    }
                                    ImGui::EndCombo();
                                }
                            };

                            drawCombo("##X1", &axisX_idx); ImGui::SameLine(); drawCombo("##Y1", &axisY_idx);
                            DrawJoystickMonitor("Primary Stick", displayState.axes[axisX_idx % displayState.axes.size()], displayState.axes[axisY_idx % displayState.axes.size()]);

                            if (displayState.axes.size() >= 4) {
                                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                                drawCombo("##X2", &axisX2_idx); ImGui::SameLine(); drawCombo("##Y2", &axisY2_idx);
                                DrawJoystickMonitor("Secondary Stick", displayState.axes[axisX2_idx % displayState.axes.size()], displayState.axes[axisY2_idx % displayState.axes.size()]);
                            }
                            ImGui::EndChild();
                            ImGui::SameLine();
                        }

                        ImGui::BeginChild("DataDisplay");

                        // Call professional axis visualization passing the handler reference
                        DrawAnalogAxes(joyHandler);

                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Buttons");
                        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                        for (int i = 0; i < (int)displayState.buttons.size(); i++) {
                            if (displayState.buttons[i]) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8f, 0.2f, 1));
                            ImGui::Button(std::to_string(i).c_str(), ImVec2(34, 34));
                            if (displayState.buttons[i]) ImGui::PopStyleColor();

                            float last_button_x2 = ImGui::GetItemRectMax().x;
                            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 34.0f;
                            if (i + 1 < (int)displayState.buttons.size() && next_button_x2 < window_visible_x2) ImGui::SameLine();
                        }

                        if (!displayState.hats.empty()) {
                            ImGui::Spacing(); ImGui::Separator();
                            ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "POV Hats");
                            for (int i = 0; i < (int)displayState.hats.size(); i++) {
                                if (i > 0) ImGui::SameLine();
                                DrawHatVisualizer(("Hat " + std::to_string(i)).c_str(), displayState.hats[i], 80.0f);
                            }
                        }
                        ImGui::EndChild();
                    } else {
                        ImGui::Text("Ready. Please select a controller to begin testing.");
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Axis Curves")) {
                    if (deviceOpened) {

                        // REMOVED: joyHandler.update();
                        // REMOVED: axisHistory push_back / pop_front logic.
                        // Data tracking is now running continuously in the background.

                        const auto& state = joyHandler.getState();

                        // --- UI Controls for Graph Tweaking ---
                        ImGui::SetNextItemWidth(200);
                        ImGui::SliderInt("Speed (Time Window)", &maxSamples, 50, 1500, "%d samples");

                        // Tooltip for the Speed/Samples slider
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Defines the number of historical data points drawn on the graph.\n"
                            "Lower values make the graph scroll faster, higher values show a longer history.");
                        }

                        ImGui::SameLine(0, 30.0f);

                        ImGui::SetNextItemWidth(200);
                        ImGui::SliderFloat("Zoom (Y-Axis)", &zoomLevel, 0.05f, 1.0f, "%.2f");

                        // Tooltip for the Zoom slider
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Adjusts the vertical scale of the graph.\n"
                            "Zoom in (lower values) to easily inspect minor sensor noise, jitter, or stick drift.");
                        }

                        ImGui::Separator();
                        ImGui::BeginChild("PlotArea");

                        for (int i = 0; i < (int)state.axes.size(); i++) {

                            // Ensure the history buffer actually exists for this axis before accessing it.
                            // Prevents crashes during frame-desyncs when switching devices rapidly.
                            if (i >= (int)axisHistory.size()) continue;

                            // Fetch the data directly from the globally updated history buffer
                            std::vector<float> data(axisHistory[i].begin(), axisHistory[i].end());

                            bool isTrigger = joyHandler.isAxisTrigger(i);
                            float scale_min = isTrigger ? 0.0f : -zoomLevel;
                            float scale_max = zoomLevel;

                            std::string labelText = "Axis " + std::to_string(i) + " History";
                            if (isTrigger) {
                                labelText += " (Trigger Mode: 0.0 to " + std::to_string(scale_max).substr(0, 4) + ")";
                            } else {
                                labelText += " (Stick Mode: " + std::to_string(scale_min).substr(0, 5) + " to " + std::to_string(scale_max).substr(0, 4) + ")";
                            }

                            ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "%s", labelText.c_str());

                            ImGui::PlotLines(("##Plot" + std::to_string(i)).c_str(),
                                             data.data(), (int)data.size(), 0, nullptr,
                                             scale_min, scale_max, ImVec2(-1, 80));

                            ImGui::Spacing();
                        }
                        ImGui::EndChild();
                    } else {
                        ImGui::Text("Ready. Please select a controller to begin testing.");
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Event Log")) {
                    if (deviceOpened) {
                        if (ImGui::Button("Clear Log", ImVec2(120, 0))) {
                            eventLog.clear();
                        }
                        ImGui::SameLine();
                        ImGui::TextDisabled("Press and release a button to record its duration.");

                        ImGui::Separator();
                        ImGui::BeginChild("EventLogRegion", ImVec2(0, 0), true);

                        // Iterate backwards to show the newest events at the top
                        for (auto it = eventLog.rbegin(); it != eventLog.rend(); ++it) {
                            ImVec4 statusColor = it->isDown ? ImVec4(0.0f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
                            ImGui::TextColored(statusColor, "[%s]", it->isDown ? "DOWN" : " UP ");
                            ImGui::SameLine();

                            if (it->isDown) {
                                ImGui::Text("%s", it->eventName.c_str());
                            } else {
                                ImGui::Text("%s held for ", it->eventName.c_str());
                                ImGui::SameLine(0, 0);
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%.1f ms", it->durationMs);
                            }
                        }

                        if (eventLog.empty()) {
                            ImGui::Text("Waiting for inputs...");
                        }

                        ImGui::EndChild();
                    } else {
                        ImGui::Text("Ready. Please select a controller to begin testing.");
                    }
                    ImGui::EndTabItem();
                }

            }

            // KEYBOARD / MOUSE TABS
            if (currentMode == AppMode::KeyboardMouse) {
                if (ImGui::BeginTabItem("Keyboard Diagnostics")) {
                    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "N-Key Rollover Check");
                    ImGui::Text("Keys Currently Held: %d", (int)keysCurrentlyHeld.size());

                    ImGui::Spacing();

                    ImGui::BeginChild("HeldKeysRegion", ImVec2(0, 60), true); // 60 pixels fixed height

                    // DEBUG: UI STRESS TEST INTERCEPTION ---
                    std::vector<SDL_Scancode> displayKeys = keysCurrentlyHeld;
                    if (debug_StressTestUI) {
                        displayKeys.clear();
                        // Simulate a massive 26-key rollover (Scancodes 4 to 29 represent A to Z)
                        for (int k = 4; k <= 29; k++) {
                            displayKeys.push_back((SDL_Scancode)k);
                        }
                    }

                    // Calculate wrapping so buttons flow nicely to the next line if many are held
                    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

                    // Use 'displayKeys' for the rendering loop
                    for (size_t i = 0; i < displayKeys.size(); i++) {
                        auto scancode = displayKeys[i];

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.8f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

                        ImGui::Button(SDL_GetScancodeName(scancode));
                        ImGui::PopStyleColor(2);

                        // Wrap to next line if the next button would go off-screen
                        float last_button_x2 = ImGui::GetItemRectMax().x;
                        float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 80.0f; // Estimate 80px per button

                        // FIX: Use displayKeys.size() instead of keysCurrentlyHeld.size()
                        if (i + 1 < displayKeys.size() && next_button_x2 < window_visible_x2) {
                            ImGui::SameLine();
                        }
                    }

                    ImGui::EndChild();

                    ImGui::Separator();

                    // --- Clear Log Button for Keyboard Events ---
                    if (ImGui::Button("Clear Log", ImVec2(120, 0))) {
                        kbEventLog.clear();
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("Press and release keys to record their duration.");

                    // --- Event Stream Render Region ---
                    ImGui::BeginChild("KBEventLog", ImVec2(0, 0), true);

                    for (auto it = kbEventLog.rbegin(); it != kbEventLog.rend(); ++it) {
                        ImVec4 statusColor = it->isDown ? ImVec4(0.0f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
                        ImGui::TextColored(statusColor, "[%s]", it->isDown ? "DOWN" : " UP ");
                        ImGui::SameLine();

                        if (it->isDown) {
                            ImGui::Text("Key: %s (Scancode: %d)", it->keyName.c_str(), it->scancode);
                        } else {
                            ImGui::Text("Key: %s (Scancode: %d) held for ", it->keyName.c_str(), it->scancode);
                            ImGui::SameLine(0, 0);
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%.1f ms", it->durationMs);
                        }
                    }

                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Mouse Diagnostics")) {

                    // --- Persistent state for diagnostics ---
                    static float maxHz = 0.0f;
                    static int accumulatedScroll = 0;

                    // Update peak Hz
                    if (mouseState.currentHz > maxHz) {
                        maxHz = mouseState.currentHz;
                    }

                    // Update accumulated scroll (to visualize continuous wheel usage)
                    if (mouseState.wheelDelta != 0) {
                        accumulatedScroll += mouseState.wheelDelta;
                        // Reset delta after reading to act as a trigger
                        mouseState.wheelDelta = 0;
                    }

                    // TOP ROW: Polling Rate & Core Stats
                    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Sensor Performance");

                    if (ImGui::Button("Reset Peak Stats")) {
                        maxHz = 0.0f;
                        accumulatedScroll = 0;
                    }
                    ImGui::SameLine();

                    // Display current and max polling rate
                    ImGui::Text("Polling Rate: ");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%.0f Hz", mouseState.currentHz);
                    ImGui::SameLine();
                    ImGui::TextDisabled("(Peak: %.0f Hz)", maxHz);

                    ImGui::Separator();
                    ImGui::Spacing();

                    // MIDDLE ROW: Buttons & Deltas
                    ImGui::BeginChild("MouseButtonsRegion", ImVec2(0, 80), false);

                    ImGui::Columns(3, "MouseDataColumns", false);

                    // Column 1: Core Buttons
                    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Main Buttons");
                    const char* btnLabels[] = { "Left", "Middle", "Right", "X1 (Back)", "X2 (Forward)" };
                    for (int i = 0; i < 3; i++) {
                        if (mouseState.buttons[i]) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.8f, 0.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                        }
                        ImGui::Button(btnLabels[i], ImVec2(60, 25));
                        if (mouseState.buttons[i]) ImGui::PopStyleColor(2);
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();

                    ImGui::NextColumn();

                    // Column 2: Extra Buttons & Scroll Wheel
                    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Auxiliary");
                    for (int i = 3; i < 5; i++) {
                        if (mouseState.buttons[i]) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.8f, 0.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                        }
                        ImGui::Button(btnLabels[i], ImVec2(95, 25));
                        if (mouseState.buttons[i]) ImGui::PopStyleColor(2);
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();
                    ImGui::Text("Scroll Wheel Accumulator: %d", accumulatedScroll);

                    ImGui::NextColumn();

                    // Column 3: Raw Movement Deltas
                    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Raw Movement Data");
                    ImGui::Text("Absolute: X: %d, Y: %d", mouseState.x, mouseState.y);
                    ImGui::Text("Delta:    X: %d, Y: %d", mouseState.deltaX, mouseState.deltaY);

                    ImGui::Columns(1);
                    ImGui::EndChild();

                    ImGui::Separator();
                    ImGui::Spacing();

                    // BOTTOM ROW: 2D Sensor Tracking Canvas
                    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Sensor Trail (Jitter & Angle Snapping Test)");
                    ImGui::TextDisabled("Move the mouse across the application window to draw the trail.");

                    // Create a drawing area that fills the remaining space
                    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                    if (canvasSize.y < 200.0f) canvasSize.y = 200.0f; // Ensure minimum height

                    ImDrawList* drawList = ImGui::GetWindowDrawList();

                    // Draw Canvas Background and Border
                    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(20, 20, 20, 255));
                    drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(100, 100, 100, 255));

                    // Draw the movement trail
                    // SDL absolute mouse coordinates are relative to the client window.
                    // We map them to the screen-space of ImGui for drawing.
                    if (mouseState.movementTrail.size() >= 2) {
                        for (size_t i = 1; i < mouseState.movementTrail.size(); i++) {

                            // Map local window coordinates to global screen coordinates
                            ImVec2 p1 = ImVec2(mouseState.movementTrail[i-1].x + canvasPos.x, mouseState.movementTrail[i-1].y + canvasPos.y);
                            ImVec2 p2 = ImVec2(mouseState.movementTrail[i].x + canvasPos.x, mouseState.movementTrail[i].y + canvasPos.y);

                            // Clamp drawing points so they don't bleed out of the canvas box
                            bool p1InBounds = (p1.x >= canvasPos.x && p1.x <= canvasPos.x + canvasSize.x && p1.y >= canvasPos.y && p1.y <= canvasPos.y + canvasSize.y);
                            bool p2InBounds = (p2.x >= canvasPos.x && p2.x <= canvasPos.x + canvasSize.x && p2.y >= canvasPos.y && p2.y <= canvasPos.y + canvasSize.y);

                            if (p1InBounds && p2InBounds) {
                                // Draw a line segment
                                drawList->AddLine(p1, p2, IM_COL32(0, 255, 128, 200), 2.0f);

                                // Draw a dot at the polling point to visualize report frequency
                                drawList->AddCircleFilled(p2, 2.0f, IM_COL32(255, 255, 255, 255));
                            }
                        }
                    }

                    // Invisible button overlay over the canvas to capture mouse input context if needed
                    ImGui::InvisibleButton("CanvasOverlay", canvasSize);

                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        ImGui::Render();
        int dw, dh; SDL_GL_GetDrawableSize(window, &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
