/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>

#include <SDL.h>
#include <SDL_ttf.h>

#include <core/Logging.h>

#include <Geometry.h>

namespace Scratch {

extern_logging_category(scratch);

class SDLContext {
public:
    SDLContext(int width, int height);
    ~SDLContext() = default;

    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    SDL_Window* window() { return m_window; }
    SDL_Renderer* renderer() { return m_renderer; }
    SDL_Cursor* arrow() { return m_arrow; }
    SDL_Cursor* input() { return m_arrow; }
    [[nodiscard]] int character_width() const { return m_fixed.character_width; }
    [[nodiscard]] int character_height() const { return m_fixed.character_height; }
    SDL_Rect render_fixed(int x, int y, std::string const& text, SDL_Color color)
    {
        return m_fixed.render_text(x, y, text, color);
    }
    SDL_Rect render_proportional(int x, int y, std::string const& text, SDL_Color color)
    {
        return m_proportional.render_text(x, y, text, color);
    }
    SDL_Rect render_fixed_right_aligned(int x, int y, std::string const& text, SDL_Color color)
    {
        return m_fixed.render_text_right_aligned(x, y, text, color);
    }
    SDL_Rect render_proportional_right_aligned(int x, int y, std::string const& text, SDL_Color color)
    {
        return m_proportional.render_text_right_aligned(x, y, text, color);
    }

private:
    struct SDLInit {
        SDLInit();
        ~SDLInit();
        bool init { false };
    };

    struct SDLTTF {
        SDLTTF();
        ~SDLTTF();
        bool init { false };
    };

    struct SDLWindow {
        SDLWindow(int width, int height);
        ~SDLWindow();
        operator SDL_Window*() const { return window; }

        SDL_Window* window { nullptr };
    };

    struct SDLRenderer {
        explicit SDLRenderer(SDLWindow const& window);
        ~SDLRenderer();
        operator SDL_Renderer*() const { return renderer; }

        SDL_Renderer* renderer;
    };

    struct SDLFont {
        SDLFont(SDLRenderer&, std::string font_name, int point_size);
        ~SDLFont();
        [[nodiscard]] std::string to_string() const { return name; }
        operator TTF_Font*() const { return font; }
        SDL_Rect render_text(int, int, std::string const& text, SDL_Color color) const;
        SDL_Rect render_text_right_aligned(int, int, std::string const& text, SDL_Color color) const;

        SDLRenderer& renderer;
        TTF_Font* font;
        std::string name;
        int size;
        int character_width;
        int character_height;
    };

    struct SDLCursor {
        SDLCursor(SDL_SystemCursor);
        ~SDLCursor();
        operator SDL_Cursor*() const { return cursor; }

        SDL_SystemCursor id;
        SDL_Cursor* cursor;
    };

    int m_width { 0 };
    int m_height { 0 };
    SDLInit m_sdl {};
    SDLTTF m_ttf {};
    SDLWindow m_window { m_width, m_height };
    SDLRenderer m_renderer { m_window };
    SDLFont m_fixed { m_renderer, "JetBrainsMono.ttf", 15 };
    SDLFont m_proportional { m_renderer, "Swansea-q3pd.ttf", 15 };
    SDLCursor m_arrow { SDL_SYSTEM_CURSOR_ARROW };
    SDLCursor m_input { SDL_SYSTEM_CURSOR_IBEAM };
};

}
