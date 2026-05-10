#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

/**
 * Helper to convert SDL_HAT values to human-readable strings (8-way support)
 */
inline const char* GetHatDirString(uint8_t value) {
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
inline int GetHatDegree(uint8_t value) {
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
