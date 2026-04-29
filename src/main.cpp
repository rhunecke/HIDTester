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
    
    // Background: Dark circle with border
    draw_list->AddCircleFilled(center, size * 0.5f, IM_COL32(25, 25, 25, 255));
    draw_list->AddCircle(center, size * 0.5f, IM_COL32(80, 80, 80, 255), 0, 2.0f);
    
    // Main axes (Cross)
    draw_list->AddLine(ImVec2(center.x - size * 0.5f, center.y), ImVec2(center.x + size * 0.5f, center.y), IM_COL32(60, 60, 60, 255));
    draw_list->AddLine(ImVec2(center.x, center.y - size * 0.5f), ImVec2(center.x, center.y + size * 0.5f), IM_COL32(60, 60, 60, 255));
    
    // Diagonal axes (X)
    float diag = size * 0.35f;
    draw_list->AddLine(ImVec2(center.x - diag, center.y - diag), ImVec2(center.x + diag, center.y + diag), IM_COL32(45, 45, 45, 255));
    draw_list->AddLine(ImVec2(center.x + diag, center.y - diag), ImVec2(center.x - diag, center.y + diag), IM_COL32(45, 45, 45, 255));

    // Calculate dot position based on hat state
    float dx = 0.0f;
    float dy = 0.0f;
    if (hatState & SDL_HAT_UP)    dy = -1.0f;
    if (hatState & SDL_HAT_DOWN)  dy =  1.0f;
    if (hatState & SDL_HAT_LEFT)  dx = -1.0f;
    if (hatState & SDL_HAT_RIGHT) dx =  1.0f;

    // Normalize for perfect diagonals
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }

    // Draw the indicator dot
    ImVec2 dot_pos = ImVec2(center.x + (dx * size * 0.4f), center.y + (dy * size * 0.4f));
    draw_list->AddCircleFilled(dot_pos, 6.0f, IM_COL32(0, 0, 0, 150));
    draw_list->AddCircleFilled(dot_pos, 5.0f, IM_COL32(0, 255, 100, 255));
    
    ImGui::Dummy(sz);
    ImGui::TextDisabled("%s", GetHatDirString(hatState));
    ImGui::EndGroup();
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) return -1;

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Official Application Name
    SDL_Window* window = SDL_CreateWindow("HID Tester - A free Joystick Testing App", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          1280, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    
    // --- SET WINDOW ICON FROM RESOURCES ---
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
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(10, 10);

    ImGui::StyleColorsDark(); 
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]  = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);

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

        // --- 1. TOP TOOLBAR ---
        ImGui::BeginChild("Toolbar", ImVec2(0, 60), true, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {
            ImGui::Text("Device:");
            ImGui::SetNextItemWidth(250);
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
            ImGui::Checkbox("Show Monitors", &showVisualizer);

            float right_offset = ImGui::GetWindowWidth() - 100.0f; 
            if (right_offset > ImGui::GetCursorPosX()) ImGui::SameLine(right_offset);
            if (ImGui::Button("About (?)")) {
                show_about_window = true;
            }

            ImGui::EndMenuBar();
        }
        ImGui::EndChild();

        // --- ABOUT MODAL WINDOW ---
        if (show_about_window) {
            ImGui::OpenPopup("About HID Tester");
        }
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("About HID Tester", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "HID Tester - A free Joystick Testing App");
            ImGui::Text("Version: %s", VERSION_STRING);
            ImGui::Separator();
            ImGui::Text("Created by: ");
            ImGui::SameLine(0, 5.0f);
            ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "rhunecke");
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGui::SetTooltip("www.roberthunecke.com");
            }
            
            if (ImGui::IsItemClicked()) {
                #ifdef _WIN32
                ShellExecuteA(NULL, "open", "https://www.roberthunecke.com", NULL, NULL, SW_SHOWNORMAL);
                #endif
            }
            ImGui::Spacing();
            ImGui::Text("License: GNU GPL v3.0");
            ImGui::TextWrapped("This program is free software: you can redistribute it and/or modify it.");
            ImGui::Spacing();

            if (ImGui::Button("GitHub Project", ImVec2(120, 0))) {
                #ifdef _WIN32
                ShellExecuteA(NULL, "open", "https://github.com/rhunecke/HIDTester", NULL, NULL, SW_SHOWNORMAL);
                #endif
            }
            ImGui::SameLine();
            if (ImGui::Button("Close", ImVec2(120, 0))) {
                show_about_window = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // --- 2. MAIN CONTENT TABS ---
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Live Test")) {
                if (deviceOpened) {
                    joyHandler.update();
                    auto& state = joyHandler.getState();

                    if (showVisualizer && state.axes.size() >= 2) {
                        ImGui::BeginChild("VisualizerArea", ImVec2(230, 0), true);
                        ImGui::Text("Stick Monitors");
                        ImGui::Separator();
                        
                        std::vector<std::string> labels;
                        for (int i = 0; i < (int)state.axes.size(); i++) labels.push_back("Axis " + std::to_string(i));
                        
                        auto drawCombo = [&](const char* id, int* idx) {
                            ImGui::SetNextItemWidth(90);
                            if (ImGui::BeginCombo(id, labels[*idx % labels.size()].c_str())) {
                                for (int i = 0; i < (int)state.axes.size(); i++) {
                                    if (ImGui::Selectable(labels[i].c_str(), *idx == i)) *idx = i;
                                }
                                ImGui::EndCombo();
                            }
                        };

                        drawCombo("##X1", &axisX_idx); ImGui::SameLine(); drawCombo("X/Y 1", &axisY_idx);
                        DrawJoystickMonitor("Primary Stick", state.axes[axisX_idx % state.axes.size()], state.axes[axisY_idx % state.axes.size()], 180.0f);
                        
                        if (state.axes.size() >= 4) {
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                            drawCombo("##X2", &axisX2_idx); ImGui::SameLine(); drawCombo("X/Y 2", &axisY2_idx);
                            DrawJoystickMonitor("Secondary Stick", state.axes[axisX2_idx % state.axes.size()], state.axes[axisY2_idx % state.axes.size()], 180.0f);
                        }
                        ImGui::EndChild();
                        ImGui::SameLine();
                    }

                    ImGui::BeginChild("DataDisplay");
                    ImGui::Text("Analog Values");
                    for (int i = 0; i < (int)state.axes.size(); i++) {
                        char buf[32]; sprintf(buf, "Axis %d: %.4f", i, state.axes[i]);
                        ImGui::ProgressBar((state.axes[i] + 1.0f) / 2.0f, ImVec2(-1, 0), buf);
                    }
                    ImGui::Separator();
                    
                    // --- RESPONSIVE BUTTON WRAPPING ---
                    ImGui::Text("Buttons");
                    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                    for (int i = 0; i < (int)state.buttons.size(); i++) {
                        if (state.buttons[i]) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.8f, 0.2f, 1));
                        ImGui::Button(std::to_string(i).c_str(), ImVec2(30, 30));
                        if (state.buttons[i]) ImGui::PopStyleColor();

                        float last_button_x2 = ImGui::GetItemRectMax().x;
                        float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 30.0f;
                        if (i + 1 < (int)state.buttons.size() && next_button_x2 < window_visible_x2) {
                            ImGui::SameLine();
                        }
                    }
                    
                    if (!state.hats.empty()) {
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Text("POV Hats (D-Pad)");
                        ImGui::Spacing();
                        for (int i = 0; i < (int)state.hats.size(); i++) {
                            if (i > 0) ImGui::SameLine();
                            char hatLabel[32];
                            sprintf(hatLabel, "Hat %d", i);
                            DrawHatVisualizer(hatLabel, state.hats[i], 70.0f);
                        }
                    }
                    ImGui::EndChild();
                } else {
                    ImGui::Text("Please connect and select a joystick.");
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Axis Curves")) {
                if (deviceOpened) {
                    joyHandler.update();
                    auto& state = joyHandler.getState();
                    if (axisHistory.size() != state.axes.size()) axisHistory.resize(state.axes.size());
                    
                    ImGui::BeginChild("PlotArea");
                    
                    // --- NEW: Calculate dynamic sample count ---
                    // Multiply the available width by a factor (e.g., 1.5f) 
                    // if you want the graph to scroll slower.
                    float avail_width = ImGui::GetContentRegionAvail().x;
                    int dynamicMaxSamples = std::max(200, (int)(avail_width * 1.0f)); 

                    for (int i = 0; i < (int)state.axes.size(); i++) {
                        axisHistory[i].push_back(state.axes[i]);
                        
                        // --- NEW: Use while-loop instead of if ---
                        // If the window is resized smaller rapidly, we might 
                        // need to pop multiple old values at once.
                        while (axisHistory[i].size() > dynamicMaxSamples) {
                            axisHistory[i].pop_front();
                        }
                        
                        std::vector<float> data(axisHistory[i].begin(), axisHistory[i].end());
                        
                        ImGui::Text("Axis %d", i);
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
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
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