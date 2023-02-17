/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <array>
#include <map>
#include <memory>

#include <SDL.h>
#include <SDL_ttf.h>

#include <core/Logging.h>

#include <Geometry.h>

using namespace Obelix;

namespace Scratch {

extern_logging_category(scratch);

class SDLContext {
public:
    enum class SDLFontFamily {
        Fixed,
        Proportional,
        Max
    };

    SDLContext(int width, int height);
    ~SDLContext() = default;

    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    void resize(int, int);
    SDL_Window* window() { return m_window; }
    SDL_Renderer* renderer() { return m_renderer; }
    SDL_Cursor* arrow() { return m_arrow; }
    SDL_Cursor* input() { return m_arrow; }
    [[nodiscard]] int character_width() const;
    [[nodiscard]] int character_height() const;
    void enlarge_font(SDLFontFamily = SDLFontFamily::Fixed);
    void shrink_font(SDLFontFamily = SDLFontFamily::Fixed);
    void reset_font(SDLFontFamily = SDLFontFamily::Fixed);
    void set_font(std::string const&, SDLFontFamily = SDLFontFamily::Fixed);
    void set_font_size(int, SDLFontFamily = SDLFontFamily::Fixed);

    SDL_Rect render_text(int x, int y, std::string const& text, SDL_Color const& color, SDLFontFamily = SDLFontFamily::Fixed) const;
    SDL_Rect render_text_right_aligned(int x, int y, std::string const& text, SDL_Color const& color, SDLFontFamily = SDLFontFamily::Fixed) const;
    SDL_Rect render_text_centered(int x, int y, std::string const& text, SDL_Color const& color, SDLFontFamily = SDLFontFamily::Fixed) const;

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

    struct SDLIMG {
        SDLIMG();
        ~SDLIMG();
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

        void set_size(int);
        void set_font(std::string const&);
        [[nodiscard]] std::string to_string() const { return name; }
        operator TTF_Font*() const { return font; }
        SDL_Rect render(int, int, std::string const& text, SDL_Color color) const;
        SDL_Rect render_right_aligned(int, int, std::string const& text, SDL_Color color) const;
        SDL_Rect render_centered(int, int, std::string const& text, SDL_Color color) const;

        SDLRenderer& renderer;
        TTF_Font *font;
        std::string name;
        int initial_size;
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
    SDLIMG m_img {};
    std::array<SDLFont, (size_t) SDLFontFamily::Max> m_fonts {
        SDLFont { m_renderer, "JetBrainsMono", 18 },
        SDLFont { m_renderer, "Swansea-q3pd", 15 }
    };
    SDLCursor m_arrow { SDL_SYSTEM_CURSOR_ARROW };
    SDLCursor m_input { SDL_SYSTEM_CURSOR_IBEAM };
};

}
