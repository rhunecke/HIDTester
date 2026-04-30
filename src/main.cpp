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
    
    ImVec2 dot_pos = ImVec2(center.x + (axisX * sz.x * 0.5f), center.y + (axisY * sz.y * 0.5f));
    draw_list->AddCircleFilled(dot_pos, 6.0f, IM_COL32(0, 0, 0, 150));
    draw_list->AddCircleFilled(dot_pos, 5.0f, IM_COL32(0, 255, 100, 255));
    
    ImGui::Dummy(sz);
    ImGui::EndGroup();
}

/**
 * Visualizes an 8-way POV Hat (D-Pad) with a radar-style crosshair
 */
void DrawHatVisualizer(const char* label, uint8_t hatState, float size = 80.0f) {
    ImGui::BeginGroup();
    ImGui::Text("%s", label);
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
    
    ImGui::Dummy(sz);
    ImGui::TextDisabled("%s", GetHatDirString(hatState));
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
 * Displays both normalized float and the 16-bit SDL-scaled integer.
 */
void DrawAnalogAxes(const JoystickState& state) {
    int numAxes = (int)state.sdlAxes.size();
    
    if (numAxes > 0) {
        // Transparent UI labeling
        ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Analog Axes (SDL 16-bit Scaled)");
        
        // Hover tooltip to educate users about hardware vs. API resolution
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Note: Generic HID APIs (like SDL/DirectInput) automatically scale all\n"
                "hardware sensor data to a 16-bit range (-32768 to 32767).\n"
                "If your device has an 8-bit or 10-bit ADC (like an Arduino Leonardo),\n"
                "these numbers are upscaled and will jump in larger increments."
            );
        }
        
        ImGui::Separator();
        ImGui::Spacing();

        for (int i = 0; i < numAxes; i++) {
            // Retrieve the 16-bit value scaled by SDL
            int16_t sdlValue = state.sdlAxes[i];
            float floatValue = state.axes[i];
            
            float normalizedLength = (static_cast<float>(sdlValue) + 32768.0f) / 65535.0f;

            ImGui::Text("Axis %d:", i);
            ImGui::SameLine(70);

            ImVec4 barColor = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);
            if (std::abs(sdlValue) > 1000) {
                barColor = ImVec4(0.0f, 0.8f, 0.3f, 1.0f);
            }

            char overlayText[32];
            snprintf(overlayText, sizeof(overlayText), "%.4f", floatValue);

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
            ImGui::ProgressBar(normalizedLength, ImVec2(-60, 16), overlayText); 
            ImGui::PopStyleColor();

            // Display the exact integer value provided by the API
            ImGui::SameLine();
            ImGui::TextDisabled("%6d", sdlValue);
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) return -1;

    const char* glsl_version = "#version 130";
    // --- Set OpenGL context version ---
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

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

    JoystickHandler joyHandler;
    static int selectedDevice = 0;
    static int axisX_idx = 0, axisY_idx = 1, axisX2_idx = 2, axisY2_idx = 3;
    static bool showVisualizer = true;
    static bool show_about_window = false; 
    bool deviceOpened = false;

    std::vector<std::deque<float>> axisHistory;

    if (SDL_NumJoysticks() > 0) deviceOpened = joyHandler.open(selectedDevice);

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("MainViewport", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        // --- TOOLBAR ---
        ImGui::BeginChild("Toolbar", ImVec2(0, 50), true, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {
            ImGui::Text("Device:");
            ImGui::SetNextItemWidth(300);
            int nJoysticks = SDL_NumJoysticks();
            const char* currentName = (nJoysticks > 0) ? SDL_JoystickNameForIndex(selectedDevice) : "No Device Detected";
            
            if (ImGui::BeginCombo("##DeviceSelector", currentName)) {
                for (int i = 0; i < nJoysticks; i++) {
                    if (ImGui::Selectable(SDL_JoystickNameForIndex(i), selectedDevice == i)) {
                        selectedDevice = i;
                        deviceOpened = joyHandler.open(selectedDevice);
                        axisHistory.clear(); 
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Show Stick Monitors", &showVisualizer);
            
            ImGui::SameLine(ImGui::GetWindowWidth() - 110);
            if (ImGui::Button("About (?)", ImVec2(90, 0))) show_about_window = true;

            ImGui::EndMenuBar();
        }
        ImGui::EndChild();

// --- ABOUT MODAL WINDOW ---
        if (show_about_window) {
            ImGui::OpenPopup("About HID Tester");
        }
        
        // Center the modal perfectly in the main viewport
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("About HID Tester", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize)) {
            
            // App Title and Version
            ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "HID Tester - A Free Joystick Testing App");
            ImGui::Text("Version: %s", VERSION_STRING);
            ImGui::Separator();
            
            // Developer Info with clickable link
            ImGui::Text("Developed by: ");
            ImGui::SameLine(0, 5.0f);
            ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "rhunecke");
            
            // Hover effect and click action for the developer name
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGui::SetTooltip("www.roberthunecke.com");
            }
            if (ImGui::IsItemClicked()) {
                // Use the cross-platform helper instead of Windows-only ShellExecuteA
                OpenWebpage("https://www.roberthunecke.com");
            }
            
            ImGui::Spacing();
            
            // License Information
            ImGui::Text("License: GNU GPL v3.0");
            ImGui::TextWrapped("This program is free software: you can redistribute it and/or modify it.");
            ImGui::Spacing();

            // Action Buttons
            if (ImGui::Button("GitHub Project", ImVec2(120, 0))) {
                // Use the cross-platform helper for the repository as well
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
            if (ImGui::BeginTabItem("Live Test")) {
                if (deviceOpened) {
                    joyHandler.update();
                    auto& state = joyHandler.getState();

                    if (showVisualizer && state.axes.size() >= 2) {
                        ImGui::BeginChild("VisualizerArea", ImVec2(240, 0), true);
                        ImGui::Text("Joystick Mapping");
                        ImGui::Separator();
                        
                        auto drawCombo = [&](const char* id, int* idx) {
                            ImGui::SetNextItemWidth(90);
                            if (ImGui::BeginCombo(id, ("Axis " + std::to_string(*idx)).c_str())) {
                                for (int i = 0; i < (int)state.axes.size(); i++) {
                                    if (ImGui::Selectable(("Axis " + std::to_string(i)).c_str(), *idx == i)) *idx = i;
                                }
                                ImGui::EndCombo();
                            }
                        };

                        drawCombo("##X1", &axisX_idx); ImGui::SameLine(); drawCombo("##Y1", &axisY_idx);
                        DrawJoystickMonitor("Primary Stick", state.axes[axisX_idx % state.axes.size()], state.axes[axisY_idx % state.axes.size()]);
                        
                        if (state.axes.size() >= 4) {
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                            drawCombo("##X2", &axisX2_idx); ImGui::SameLine(); drawCombo("##Y2", &axisY2_idx);
                            DrawJoystickMonitor("Secondary Stick", state.axes[axisX2_idx % state.axes.size()], state.axes[axisY2_idx % state.axes.size()]);
                        }
                        ImGui::EndChild();
                        ImGui::SameLine();
                    }

                    ImGui::BeginChild("DataDisplay");
                    
                    // Call professional axis visualization
                    DrawAnalogAxes(state);
                    
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Buttons");
                    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                    for (int i = 0; i < (int)state.buttons.size(); i++) {
                        if (state.buttons[i]) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8f, 0.2f, 1));
                        ImGui::Button(std::to_string(i).c_str(), ImVec2(34, 34));
                        if (state.buttons[i]) ImGui::PopStyleColor();

                        float last_button_x2 = ImGui::GetItemRectMax().x;
                        float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 34.0f;
                        if (i + 1 < (int)state.buttons.size() && next_button_x2 < window_visible_x2) ImGui::SameLine();
                    }
                    
                    if (!state.hats.empty()) {
                        ImGui::Spacing(); ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "POV Hats");
                        for (int i = 0; i < (int)state.hats.size(); i++) {
                            if (i > 0) ImGui::SameLine();
                            DrawHatVisualizer(("Hat " + std::to_string(i)).c_str(), state.hats[i], 80.0f);
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
                    joyHandler.update();
                    auto& state = joyHandler.getState();
                    if (axisHistory.size() != state.axes.size()) axisHistory.resize(state.axes.size());
                    
                    ImGui::BeginChild("PlotArea");
                    float avail_width = ImGui::GetContentRegionAvail().x;
                    int dynamicMaxSamples = std::max(200, (int)(avail_width * 1.0f)); 

                    for (int i = 0; i < (int)state.axes.size(); i++) {
                        axisHistory[i].push_back(state.axes[i]);
                        while (axisHistory[i].size() > dynamicMaxSamples) axisHistory[i].pop_front();
                        
                        std::vector<float> data(axisHistory[i].begin(), axisHistory[i].end());
                        ImGui::Text("Axis %d History", i);
                        ImGui::PlotLines(("##Plot" + std::to_string(i)).c_str(), data.data(), (int)data.size(), 0, nullptr, -1.0f, 1.0f, ImVec2(-1, 80));
                    }
                    ImGui::EndChild();
                }
                ImGui::EndTabItem();
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