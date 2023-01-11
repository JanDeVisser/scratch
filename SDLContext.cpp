/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <sstream>

#include <SDL2_gfxPrimitives.h>
#include <SDL_ttf.h>

#include <SDLContext.h>

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
    , size(point_size)
{
    if (font = TTF_OpenFont(name.c_str(), point_size); font == nullptr)
        fatal("Could not load font '{}'", name);
    if (TTF_SizeUTF8(font, "W", &character_width, &character_height) != 0)
        fatal("Error getting size of text: {}", TTF_GetError());
    if (character_height = TTF_FontHeight(font); character_height < 0)
        fatal("Error getting font height: {}", TTF_GetError());
    debug(scratch, "Opened font '{}' w/ character size {}x{}", name, character_width, character_height);
}

SDLContext::SDLFont::~SDLFont()
{
    debug(scratch, "Closing font '{}'", name);
    if (font)
        TTF_CloseFont(font);
}

SDL_Rect SDLContext::SDLFont::render_text(int x, int y, std::string const& text, SDL_Color color) const
{
    SDL_Rect rect { x, y, 0, 0 };
    if (text.empty())
        return rect;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface)
        fatal("Error rendering text: {}", TTF_GetError());
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(texture, nullptr, nullptr, &rect.w, &rect.h);
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    return rect;
}

SDL_Rect SDLContext::SDLFont::render_text_right_aligned(int x, int y, std::string const& text, SDL_Color color) const
{
    SDL_Rect rect { x, y, 0, 0 };
    if (text.empty())
        return rect;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface)
        fatal("Error rendering text: {}", TTF_GetError());
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(texture, nullptr, nullptr, &rect.w, &rect.h);
    rect.x -= rect.w;
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    return rect;
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
//    m_window.initialize(width, height);
    if (!TTF_FontFaceIsFixedWidth(m_fixed.font))
        fatal("Font '{}' is proportional", m_fixed.name);
    SDL_ShowCursor(1);
}

}
