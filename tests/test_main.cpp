#include <SDL3/SDL.h>

#include <iostream>
#include <string>

#include "HatUtils.h"

namespace {

bool Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

bool TestHatHelpers() {
    return
        Expect(std::string(GetHatDirString(SDL_HAT_CENTERED)) == "Centered", "Centered hat label mismatch") &&
        Expect(std::string(GetHatDirString(SDL_HAT_RIGHTDOWN)) == "Down-Right", "Diagonal hat label mismatch") &&
        Expect(GetHatDegree(SDL_HAT_UP) == 0, "Up hat degree mismatch") &&
        Expect(GetHatDegree(SDL_HAT_LEFTDOWN) == 225, "Left-down hat degree mismatch") &&
        Expect(GetHatDegree(SDL_HAT_CENTERED) == -1, "Centered hat degree mismatch");
}

bool TestSDL3MigrationSmoke() {
    if (!Expect(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD), SDL_GetError())) {
        return false;
    }

    SDL_Window* window = SDL_CreateWindow("HIDTester SDL3 smoke test", 320, 240, SDL_WINDOW_HIDDEN);
    if (!Expect(window != nullptr, SDL_GetError())) {
        SDL_Quit();
        return false;
    }

    int joystickCount = 0;
    SDL_JoystickID* joystickIds = SDL_GetJoysticks(&joystickCount);
    if (!Expect(joystickCount >= 0, "Joystick enumeration returned a negative count")) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    if (joystickIds) {
        SDL_free(joystickIds);
    }

    SDL_Event pushEvent = {};
    pushEvent.type = SDL_EVENT_QUIT;
    if (!Expect(SDL_PushEvent(&pushEvent), SDL_GetError())) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    bool sawQuitEvent = false;
    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            sawQuitEvent = true;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return Expect(sawQuitEvent, "Did not receive the SDL3 quit event");
}

} // namespace

int main() {
    if (!TestHatHelpers()) {
        return 1;
    }

    if (!TestSDL3MigrationSmoke()) {
        return 1;
    }

    return 0;
}
