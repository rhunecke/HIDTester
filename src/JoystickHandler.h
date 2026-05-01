#pragma once
#include <SDL.h>
#include <vector>
#include <string>
#include <cmath> // Required for std::abs

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

            // --- Auto-Detection Heuristic for Triggers ---
            // Force an immediate update to grab the real physical state 
            // the moment the device is initialized.
            SDL_JoystickUpdate(); 
            
            for (int i = 0; i < numAxes; i++) {
                int16_t initialVal = SDL_JoystickGetAxis(joystick, i);
                
                // If the axis is physically resting at its absolute minimum 
                // (or very close to it to account for hardware jitter) upon connection, 
                // it is highly likely a unidirectional trigger or throttle.
                if (initialVal <= -32000) {
                    state.axisIsTrigger[i] = true;
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
            
            // Store the unaltered raw value for the diagnostic UI (Progress bars)
            state.sdlAxes[i] = rawSdlValue;           
            
            if (state.axisIsTrigger[i]) {
                // ==========================================
                // TRIGGER LOGIC (Unidirectional: 0.0 to 1.0)
                // ==========================================
                
                // 1. Normalize raw value to a float between 0.0f and 1.0f
                float normalizedValue = (rawSdlValue + 32768.0f) / 65535.0f;
                
                // 2. Normalize the deadzone limit to the 0.0 - 1.0 scale
                float dzFloat = deadzoneLimit / 65535.0f;
                
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
};