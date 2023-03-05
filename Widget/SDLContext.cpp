/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <sstream>

#include <SDL2_gfxPrimitives.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "SDLContext.h"

using namespace Obelix;

namespace Scratch {

SDLContext::SDLInit::SDLInit()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        fatal("Failed to initialize SDL system");
    init = true;
    debug(scratch, "Initialized SDL system");
}

SDLContext::SDLInit::~SDLInit()
{
    debug(scratch, "Terminating SDL system");
    if (init) {
        // primitivePurge();  Probably old. What did this do?
        SDL_Quit();
    }
}

SDLContext::SDLTTF::SDLTTF()
{
    if (TTF_Init() < 0)
        fatal("Failed to initialize SDL TTF system");
    init = true;
    debug(scratch, "Initialized SDL TTF system");
}

SDLContext::SDLTTF::~SDLTTF()
{
    debug(scratch, "Terminating SDL TTF system");
    if (init)
        TTF_Quit();
}

SDLContext::SDLIMG::SDLIMG()
{
    if (IMG_Init(IMG_INIT_PNG) < 0)
        fatal("Failed to initialize SDL IMG system");
    init = true;
    debug(scratch, "Initialized SDL IMG system");
}

SDLContext::SDLIMG::~SDLIMG()
{
    debug(scratch, "Terminating SDL IMG system");
    if (init)
        IMG_Quit();
}

SDLContext::SDLWindow::SDLWindow(int width, int height)
{
    debug(scratch, "Creating SDL window with size {}x{}", width, height);
    if (window = SDL_CreateWindow(
            "Scratch",
            SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0),
            width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        window == nullptr)
        fatal("Could not create SDL window");
    SDL_SetWindowMinimumSize(window, width, height);
    debug(scratch, "Initialized SDL window");
}

SDLContext::SDLWindow::~SDLWindow()
{
    debug(scratch, "Destroying SDL window");
    if (window)
        SDL_DestroyWindow(window);
}

SDLContext::SDLRenderer::SDLRenderer(SDLWindow const& window)
{
    if (renderer = SDL_GetRenderer(window); renderer == nullptr) {
        if (renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED); renderer == nullptr)
            fatal("Could not create SDL renderer");
    }
    debug(scratch, "SDL renderer initialized");
}

SDLContext::SDLRenderer::~SDLRenderer()
{
    debug(scratch, "Destroying SDL renderer");
    if (renderer)
        SDL_DestroyRenderer(renderer);
}

SDLContext::SDLFont::SDLFont(SDLRenderer& renderer, std::string font_name, int point_size)
    : renderer(renderer)
    , name(std::move(font_name))
    , initial_size(point_size)
    , size(point_size)
{
    set_font(name);
    debug(scratch, "Opened font '{}' w/ character size {}x{}", name, character_width, character_height);
}

SDLContext::SDLFont::~SDLFont()
{
    if (font)
        TTF_CloseFont(font);
}

void SDLContext::SDLFont::set_size(int point_size)
{
    size = point_size;
    if (TTF_SetFontSize(font, size) != 0)
        fatal("Error setting size of font {} to {}: {}", name, size, TTF_GetError());
    if (TTF_SizeUTF8(font, "W", &character_width, &character_height) != 0)
        fatal("Error getting size of text: {}", TTF_GetError());
    if (character_height = TTF_FontHeight(font); character_height < 0)
        fatal("Error getting font height: {}", TTF_GetError());
}

void SDLContext::SDLFont::set_font(std::string const& font_name)
{
    name = font_name;
    if (font = TTF_OpenFont(format("fonts/{}.ttf", name).c_str(), size); font == nullptr)
        fatal("Could not load font '{}': {}", name, TTF_GetError());
    set_size(size);
}

SDL_Rect SDLContext::SDLFont::render(int x, int y, std::string const& text, SDL_Color color) const
{
    SDL_Rect rect { x, y, 0, 0 };
    if (text.empty()) {
        return rect;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface)
        fatal("Error rendering text: {}", TTF_GetError());
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(texture, nullptr, nullptr, &rect.w, &rect.h);
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_DestroyTexture(texture);
    return rect;
}

SDL_Rect SDLContext::SDLFont::render_right_aligned(int x, int y, std::string const& text, SDL_Color color) const
{
    SDL_Rect rect { x, y, 0, 0 };
    if (text.empty()) {
        return rect;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface)
        fatal("Error rendering text: {}", TTF_GetError());
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(texture, nullptr, nullptr, &rect.w, &rect.h);
    rect.x -= rect.w;
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_DestroyTexture(texture);
    return rect;
}

SDL_Rect SDLContext::SDLFont::render_centered(int x, int y, std::string const& text, SDL_Color color) const
{
    SDL_Rect rect { x, y, 0, 0 };
    if (text.empty()) {
        return rect;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface)
        fatal("Error rendering text: {}", TTF_GetError());
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(texture, nullptr, nullptr, &rect.w, &rect.h);
    rect.x -= rect.w / 2;
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_DestroyTexture(texture);
    return rect;
}

int SDLContext::SDLFont::text_width(std::string const& text) const
{
    int width, height;
    if (TTF_SizeUTF8(font, text.c_str(), &width, &height) != 0)
        fatal("Error getting text width: {}", TTF_GetError());
    return width;
}

SDLContext::SDLCursor::SDLCursor(SDL_SystemCursor cursor_id)
    : id(cursor_id)
{
    if (cursor = SDL_CreateSystemCursor(id); cursor == nullptr)
        fatal("Could not initialize SDL cursor '{}'", (int)id);
    debug(scratch, "Initialized SDL cursor '{}'", (int)id);
}

SDLContext::SDLCursor::~SDLCursor()
{
    debug(scratch, "Destroying SDL cursor '{}'", (int)id);
    if (cursor)
        SDL_FreeCursor(cursor);
}

SDLContext::SDLContext(int width, int height)
    : m_width(width)
    , m_height(height)
{
    if (!TTF_FontFaceIsFixedWidth(m_fonts[(size_t)SDLFontFamily::Fixed].font))
        fatal("Font '{}' is proportional", m_fonts[(size_t)SDLFontFamily::Fixed].name);
    SDL_ShowCursor(1);
}

void SDLContext::resize(int width, int height)
{
    m_width = width;
    m_height = height;
}

int SDLContext::character_width() const
{
    return m_fonts[(size_t)SDLFontFamily::Fixed].character_width;
}

int SDLContext::character_height() const
{
    return m_fonts[(size_t)SDLFontFamily::Fixed].character_height;
}

void SDLContext::enlarge_font(SDLFontFamily family)
{
    set_font_size(m_fonts[(size_t)family].size * 1.2, family);
}

void SDLContext::shrink_font(SDLFontFamily family)
{
    set_font_size(m_fonts[(size_t)family].size / 1.2, family);
}

void SDLContext::reset_font(SDLFontFamily family)
{
    set_font_size(m_fonts[(size_t)family].initial_size, family);
}

void SDLContext::set_font(std::string const& name, SDLFontFamily family)
{
    m_fonts[(size_t)family].set_font(name);
}

void SDLContext::set_font_size(int points, SDLFontFamily family)
{
    m_fonts[(size_t)family].set_size(points);
}

SDL_Rect SDLContext::render_text(int x, int y, std::string const& text, SDL_Color const& color, SDLFontFamily family) const
{
    return m_fonts[(size_t)family].render(x, y, text, color);
}

SDL_Rect SDLContext::render_text_right_aligned(int x, int y, std::string const& text, SDL_Color const& color, SDLFontFamily family) const
{
    return m_fonts[(size_t)family].render_right_aligned(x, y, text, color);
}

SDL_Rect SDLContext::render_text_centered(int x, int y, std::string const& text, SDL_Color const& color, SDLFontFamily family) const
{
    return m_fonts[(size_t)family].render_centered(x, y, text, color);
}

int SDLContext::text_width(std::string const& text, SDLFontFamily family) const
{
    return m_fonts[(size_t)family].text_width(text);
}

}
