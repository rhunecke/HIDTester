#pragma once
#include <SDL.h>
#include <vector>
#include <string>

struct JoystickState {
    std::vector<float> axes;
    std::vector<bool> buttons;
    std::vector<uint8_t> hats;
};

class JoystickHandler {
public:
    JoystickHandler() : joystick(nullptr) {}
    ~JoystickHandler() { close(); }

    bool open(int deviceIndex) {
        close();
        joystick = SDL_JoystickOpen(deviceIndex);
        if (joystick) {
            state.axes.assign(SDL_JoystickNumAxes(joystick), 0.0f);
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
        for (int i = 0; i < (int)state.axes.size(); i++) {
            // Normalisierung auf -1.0 bis 1.0
            state.axes[i] = SDL_JoystickGetAxis(joystick, i) / 32767.0f;
        }
        for (int i = 0; i < (int)state.buttons.size(); i++) {
            state.buttons[i] = SDL_JoystickGetButton(joystick, i) != 0;
        }
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