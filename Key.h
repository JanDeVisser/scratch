/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include <SDL.h>

namespace Scratch {

struct SDLKey {
    SDL_Keycode sym;
    uint16_t mod;

    SDLKey(SDL_Keycode, uint32_t);
    SDLKey(SDL_Keysym const&);

    int operator<=>(SDLKey const& other) const;
    [[nodiscard]] std::string to_string() const;
};

} // Scratch
