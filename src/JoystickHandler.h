#pragma once
#include <SDL.h>
#include <vector>
#include <string>
#include <cmath>     // Required for std::abs
#include <algorithm> // Required for std::max

// --- Data Model for the Joystick State ---
struct JoystickState {
    std::vector<float> axes;       // Normalized and deadzone-processed values (-1.0f to 1.0f or 0.0f to 1.0f)
    std::vector<int16_t> sdlAxes;  // SDL-scaled 16-bit values (-32768 to 32767). Note: Not true hardware raw!
    std::vector<bool> buttons;     // Button states (true = pressed)
    std::vector<uint8_t> hats;     // POV Hat states (e.g., SDL_HAT_UP, SDL_HAT_CENTERED)
    std::vector<bool> axisIsTrigger; // True if the axis is unidirectional (e.g., a trigger)
};

class JoystickHandler {
public:
    JoystickHandler() : joystick(nullptr), deadzoneLimit(0) {}
    
    ~JoystickHandler() { 
        close(); 
    }

    // Set the software deadzone threshold (0 to 32767)
    void setDeadzone(int16_t limit) {
        deadzoneLimit = limit;
    }

    // Toggles whether a specific axis should be treated as a Trigger
    void setAxisTriggerMode(int axisIndex, bool isTrigger) {
        if (axisIndex >= 0 && axisIndex < state.axisIsTrigger.size()) {
            state.axisIsTrigger[axisIndex] = isTrigger;
        }
    }

    // Returns the current mode of an axis
    bool isAxisTrigger(int axisIndex) const {
        if (axisIndex >= 0 && axisIndex < state.axisIsTrigger.size()) {
            return state.axisIsTrigger[axisIndex];
        }
        return false;
    }

    bool open(int deviceIndex) {
        close();
        joystick = SDL_JoystickOpen(deviceIndex);
        
        if (joystick) {
            int numAxes = SDL_JoystickNumAxes(joystick);
            state.axes.assign(numAxes, 0.0f);
            state.sdlAxes.assign(numAxes, 0);
            state.buttons.assign(SDL_JoystickNumButtons(joystick), false);
            state.hats.assign(SDL_JoystickNumHats(joystick), SDL_HAT_CENTERED);
            
            // Initialize all axes as default bidirectional sticks
            state.axisIsTrigger.assign(numAxes, false);

            // Mark all axes as pending for the event-driven auto-detection
            axisAutoDetectPending.assign(numAxes, true);

            // --- Auto-Detection Heuristic for Triggers ---
            // Force an immediate update to grab the real physical state 
            // the moment the device is initialized.
            SDL_JoystickUpdate(); 
            
            for (int i = 0; i < numAxes; i++) {
                int16_t initialVal = 0;
                
                // Try to get the initial state if the driver provides it immediately
                if (SDL_JoystickGetAxisInitialState(joystick, i, &initialVal)) {
                    if (initialVal != 0) {
#ifndef __APPLE__
                        // Windows & Linux: Triggers rest at absolute minimum (-32768).
                        if (initialVal <= -32000) {
                            state.axisIsTrigger[i] = true;
                        }
#else
                // macOS: Both Triggers and Sticks rest at 0.
                // It is impossible to safely distinguish them just by looking at the initial value.
                // We leave it false. The macOS user must manually check the "Trg" box in the UI.
#endif
                        axisAutoDetectPending[i] = false; // Detection finished for this axis
                    }
                }
            }
            
            return true;
        }
        return false;
    }

    void close() {
        if (joystick) {
            SDL_JoystickClose(joystick);
            joystick = nullptr;
        }
    }

    void update() {
        if (!joystick) return;
        
        // Process analog axes
        for (int i = 0; i < (int)state.axes.size(); i++) {
            // Read the raw API value
            int16_t rawSdlValue = SDL_JoystickGetAxis(joystick, i);

            // --- EVENT-DRIVEN AUTO-DETECT ---
            // If the OS/Driver delayed the initial report, we wait until the axis shows movement.
            // A resting trigger will immediately jump to -32768 on its first true USB packet.
            if (axisAutoDetectPending[i] && rawSdlValue != 0) {
#ifndef __APPLE__
                if (rawSdlValue <= -32000) {
                    state.axisIsTrigger[i] = true;
                }
#endif
                axisAutoDetectPending[i] = false; // Decision made, lock it in.
            }
            
            // Store the unaltered raw value for the diagnostic UI (Progress bars)
            state.sdlAxes[i] = rawSdlValue;           
            
            if (state.axisIsTrigger[i]) {
                // ==========================================
                // TRIGGER LOGIC (Unidirectional: 0.0 to 1.0)
                // ==========================================
                
                float normalizedValue = 0.0f;
                float dzFloat = 0.0f;

#ifdef __APPLE__
                // macOS API reports triggers purely from 0 to 32767.
                // std::max ensures slight negative jitter around 0 doesn't break the math.
                normalizedValue = std::max(0.0f, (float)rawSdlValue) / 32767.0f;
                dzFloat = deadzoneLimit / 32767.0f;
#else
                // Windows/Linux API reports triggers from -32768 to 32767.
                normalizedValue = (rawSdlValue + 32768.0f) / 65535.0f;
                dzFloat = deadzoneLimit / 65535.0f;
#endif
                
                float finalOutput = 0.0f;
                
                // 3. Apply Scaled Deadzone Math starting from the bottom
                if (normalizedValue > dzFloat) {
                    finalOutput = (normalizedValue - dzFloat) / (1.0f - dzFloat);
                }

                // Store the smoothed, deadzone-processed value
                state.axes[i] = finalOutput;

            } else {
                // ==========================================
                // STICK LOGIC (Bidirectional: -1.0 to 1.0)
                // ==========================================
                
                // Normalize raw value to a float between -1.0f and 1.0f
                float normalizedValue = rawSdlValue / 32767.0f;
                
                // Normalize the deadzone limit to a float
                float dzFloat = deadzoneLimit / 32767.0f;
                
                float finalOutput = 0.0f;
                
                // Apply Scaled Deadzone Math (Professional Game Engine Style)
                if (std::abs(normalizedValue) > dzFloat) {
                    // Determine direction (positive or negative axis movement)
                    float sign = (normalizedValue > 0.0f) ? 1.0f : -1.0f;
                    
                    // Rescale the remaining physical range to a smooth 0.0 to 1.0 logical range
                    finalOutput = sign * ((std::abs(normalizedValue) - dzFloat) / (1.0f - dzFloat));
                }

                // Store the smoothed, deadzone-processed value
                state.axes[i] = finalOutput;   
            }
        }
        
        // Process digital buttons
        for (int i = 0; i < (int)state.buttons.size(); i++) {
            state.buttons[i] = SDL_JoystickGetButton(joystick, i) != 0;
        }
        
        // Process POV Hats (D-Pads)
        for (int i = 0; i < (int)state.hats.size(); i++) {
            state.hats[i] = SDL_JoystickGetHat(joystick, i);
        }
    }

    const JoystickState& getState() const { return state; }
    std::string getName() const { return joystick ? SDL_JoystickName(joystick) : "None"; }
    bool isOpen() const { return joystick != nullptr; }

private:
    SDL_Joystick* joystick;
    JoystickState state;
    int16_t deadzoneLimit; 
    std::vector<bool> axisAutoDetectPending; // Tracks which axes still need initial classification
};