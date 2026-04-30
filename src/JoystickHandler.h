#pragma once
#include <SDL.h>
#include <vector>
#include <string>

// --- Data Model for the Joystick State ---
struct JoystickState {
    std::vector<float> axes;       // Normalized axis values (-1.0f to 1.0f) for plotting
    std::vector<int16_t> rawAxes;  // Exact raw hardware values (-32768 to 32767) for precise calibration UI
    std::vector<bool> buttons;     // Button states (true = pressed)
    std::vector<uint8_t> hats;     // POV Hat states (e.g., SDL_HAT_UP, SDL_HAT_CENTERED)
};

class JoystickHandler {
public:
    JoystickHandler() : joystick(nullptr) {}
    
    ~JoystickHandler() { 
        close(); 
    }

    /**
     * Opens the joystick at the specified device index.
     * Initializes all state vectors to match the device capabilities.
     */
    bool open(int deviceIndex) {
        close();
        joystick = SDL_JoystickOpen(deviceIndex);
        
        if (joystick) {
            int numAxes = SDL_JoystickNumAxes(joystick);
            state.axes.assign(numAxes, 0.0f);
            state.rawAxes.assign(numAxes, 0);
            state.buttons.assign(SDL_JoystickNumButtons(joystick), false);
            state.hats.assign(SDL_JoystickNumHats(joystick), SDL_HAT_CENTERED);
            
            return true;
        }
        return false;
    }

    /**
     * Safely closes the joystick and releases SDL resources.
     */
    void close() {
        if (joystick) {
            SDL_JoystickClose(joystick);
            joystick = nullptr;
        }
    }

    /**
     * Polls the latest hardware data from SDL and updates the internal state.
     * Should be called once per frame before rendering the UI.
     */
    void update() {
        if (!joystick) return;
        
        // Process analog axes
        for (int i = 0; i < (int)state.axes.size(); i++) {
            int16_t rawValue = SDL_JoystickGetAxis(joystick, i);
            state.rawAxes[i] = rawValue;                 // Store absolute raw value
            state.axes[i] = rawValue / 32767.0f;         // Store normalized value
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

    // --- Getters ---
    const JoystickState& getState() const { return state; }
    std::string getName() const { return joystick ? SDL_JoystickName(joystick) : "None"; }
    bool isOpen() const { return joystick != nullptr; }

private:
    SDL_Joystick* joystick;
    JoystickState state;
};