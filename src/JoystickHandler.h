#pragma once
#include <SDL.h>
#include <vector>
#include <string>

// --- Data Model for the Joystick State ---
struct JoystickState {
    std::vector<float> axes;       // Normalized axis values (-1.0f to 1.0f) for plotting
    std::vector<int16_t> sdlAxes;  // SDL-scaled 16-bit values (-32768 to 32767). Note: Not true hardware raw!
    std::vector<bool> buttons;     // Button states (true = pressed)
    std::vector<uint8_t> hats;     // POV Hat states (e.g., SDL_HAT_UP, SDL_HAT_CENTERED)
};

class JoystickHandler {
public:
    JoystickHandler() : joystick(nullptr) {}
    
    ~JoystickHandler() { 
        close(); 
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
            // SDL always scales generic HID inputs to a 16-bit signed integer range
            int16_t sdlScaledValue = SDL_JoystickGetAxis(joystick, i);
            state.sdlAxes[i] = sdlScaledValue;           
            state.axes[i] = sdlScaledValue / 32767.0f;   
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
};