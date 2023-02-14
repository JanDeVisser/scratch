/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <map>
#include <sstream>

#include <Key.h>

namespace Scratch {

static uint32_t normalize_mod(uint32_t mod)
{
    uint32_t ret = 0;
    if (mod & KMOD_SHIFT)
        ret |= KMOD_SHIFT;
    if (mod & KMOD_CTRL)
        ret |= KMOD_CTRL;
    if (mod & KMOD_ALT)
        ret |= KMOD_ALT;
    if (mod & KMOD_GUI)
        ret |= KMOD_GUI;
    return ret;
}

SDLKey::SDLKey(SDL_Keycode the_sym, uint32_t the_mod)
    : sym(the_sym)
{
    mod = normalize_mod(the_mod);
}

SDLKey::SDLKey(SDL_Keysym const& key)
    : sym((key.sym < 'A' || key.sym > 'Z') ? key.sym : key.sym + 32)
{
    mod = normalize_mod(key.mod);
}

int SDLKey::operator<=>(SDLKey const& other) const
{
    if (sym != other.sym)
        return sym - other.sym;
    if (mod == other.mod)
        return 0;
    return mod - other.mod;
}

std::string SDLKey::to_string() const
{
    static std::map<SDL_Keycode, std::string> keymap = {
        { SDLK_UP, "UP" },
        { SDLK_DOWN, "DOWN" },
        { SDLK_LEFT, "LEFT" },
        { SDLK_RIGHT, "RIGHT" },
        { SDLK_PAGEUP, "PGUP" },
        { SDLK_PAGEDOWN, "PGDN" },
        { SDLK_HOME, "HOME" },
        { SDLK_END, "END" },
        { SDLK_INSERT, "INS" },
        { SDLK_DELETE, "DEL" },
        { SDLK_TAB, "TAB" },
        { SDLK_RETURN, "RET" },
        { SDLK_KP_ENTER, "RET" },
        { SDLK_ESCAPE, "ESC" },
        { SDLK_UNKNOWN, "" },
    };

    std::stringstream ss;
    if (mod & KMOD_GUI)
        ss << "M-";
    if (mod & KMOD_ALT)
        ss << "A-";
    if (mod & KMOD_CTRL)
        ss << "C-";
    if (mod & KMOD_SHIFT) {
        ss << "S-";
    }
    if ((sym >= ' ') && (sym <= '~')) {
        ss << (char)sym;
        return ss.str();
    }
    if ((sym >= SDLK_F1) && (sym <= SDLK_F12)) {
        ss << "F" << sym - SDLK_F1 + 1;
        return ss.str();
    }
    if (keymap.contains(sym)) {
        ss << keymap[sym];
        return ss.str();
    }
    ss << std::hex << (uint32_t)sym;
    return ss.str();
}

} // Scratch
