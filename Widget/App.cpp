/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <array>
#include <cstdio>
#include <thread>

#include <SDL2_gfxPrimitives.h>

#include "App.h"
#include "App/Text.h"
#include "Commands/CommandHandler.h"
#include "SDLContext.h"

namespace Scratch {

App* App::s_app { nullptr };

App& App::instance()
{
    oassert(s_app != nullptr, "No App instantiated");
    return *s_app;
}

App::App(std::string name, SDLContext* ctx)
    : Layout(ContainerOrientation::Vertical)
    , m_name(std::move(name))
    , m_palette(DarkPalette())
    , m_context(ctx)
{
    oassert(s_app == nullptr, "App is a singleton");
    s_app = this;
}

void App::add_modal(Widget* widget)
{
    m_modals.emplace_back(widget);
}

Widget* App::modal()
{
    if (m_modals.empty())
        return nullptr;
    return m_modals.back().get();
}

void App::dismiss_modal()
{
    if (!m_modals.empty()) {
        m_modals.pop_back();
    }
}

Widget* App::focus()
{
    return m_focus;
}

void App::focus(Widget *widget)
{
    // Call event handlers for loss and gain of focus
    m_focus = widget;
}

SDL_Renderer* App::renderer()
{
    return m_context->renderer();
}

void App::enlarge_font()
{
    context()->enlarge_font(SDLContext::SDLFontFamily::Fixed);
    container().resize({ 0, 0, m_width, m_height });
}

void App::shrink_font()
{
    context()->shrink_font(SDLContext::SDLFontFamily::Fixed);
    container().resize({ 0, 0, m_width, m_height });
}

void App::reset_font()
{
    context()->reset_font(SDLContext::SDLFontFamily::Fixed);
    container().resize({ 0, 0, m_width, m_height });
}

void App::set_font(std::string const& name)
{
    context()->set_font(name, SDLContext::SDLFontFamily::Fixed);
    container().resize({ 0, 0, m_width, m_height });
}

void App::render()
{
    m_frameCount++;

    SDL_SetRenderDrawColor(renderer(), 0x2e, 0x32, 0x38, 0xff);
    SDL_RenderClear(renderer());
    for (auto const& c : components()) {
        c->render();
    }
    if (m_modals.empty()) {
        if (!m_pending_commands.empty()) {
            auto cmd = m_pending_commands.front();
            add_modal(new CommandHandler(cmd));
            m_pending_commands.pop_front();
        }
    } else {
        for (auto& m : m_modals) {
            m->render();
        }
    }
    SDL_RenderPresent(renderer());
}

void App::resize(Box const& outline)
{
    context()->resize(m_width, m_height);
    Layout::resize(outline);
}

int App::fps() const
{
    if (m_last_render_time.count() < 0.001)
        return 0;
    return (int) (1.0 / m_last_render_time.count());
}


void App::schedule(ScheduledCommand cmd)
{
    m_pending_commands.push_back(cmd);
}

int App::width() const
{
    return context()->width();
}

int App::height() const
{
    return context()->height();
}

intptr_t App::active() const
{
    return m_active;
}

void App::active(intptr_t val)
{
    m_active = val;
}

/*
 * Equivalent to:
 *    uint8_t a = (c >> 24) & 0xff;
 *    uint8_t b = (c >> 16) & 0xff;
 *    uint8_t g = (c >> 8) & 0xff;
 *    uint8_t r = c & 0xff;
 *    return { r, g, b, a };
 */
SDL_Color App::color(PaletteIndex color)
{
    static unsigned s_ansicolors[] = {
        0xff000000, // ANSIBlack,
        0xff0000cc, // ANSIRed,
        0xff069a4e, // ANSIGreen,
        0xff00a0c4, // ANSIYellow,
        0xffcf9f72, // ANSIBlue,
        0xff7b5075, // ANSIMagenta,
        0xff9a9806, // ANSICyan,
        0xffcfd7d3, // ANSIWhite,
        0xff535755, // ANSIBrightBlack,
        0xff2929ef, // ANSIBrightRed,
        0xff34e28a, // ANSIBrightGreen,
        0xff4fe9fc, // ANSIBrightYellow,
        0xffffaf32, // ANSIBrightBlue,
        0xffa87fad, // ANSIBrightMagenta,
        0xffe2e234, // ANSIBrightCyan,
        0xffffffff, // ANSIBrightWhite,
    };
    uint32_t c;
    if (color >= PaletteIndex::ANSIBlack && color <= PaletteIndex::ANSIBrightWhite)
        c = s_ansicolors[(size_t)color - (size_t)PaletteIndex::ANSIBlack];
    else
        c = m_palette[(size_t)color];
    return *((SDL_Color*)&c);
}

void App::event_loop()
{
    SDL_Event evt;

    auto start_render = std::chrono::steady_clock::now();
    while (!m_quit) {

        // Processes events.
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
            case SDL_QUIT: {
                m_quit = true;
            } break;
            case SDL_WINDOWEVENT: {
                switch (evt.window.event) {
                case SDL_WINDOWEVENT_SHOWN:
                case SDL_WINDOWEVENT_RESIZED: {
                    SDL_GetRendererOutputSize(renderer(), &m_width, &m_height);
                    resize({ 0, 0, m_width, m_height });
                } break;
                }
            } break;
            case SDL_KEYDOWN: {
                m_last_key = evt.key.keysym;
                Widget *target = this;
                if (auto m = modal(); m != nullptr) {
                    target = m;
                }
                target->dispatch(evt.key.keysym);
            } break;
            case SDL_TEXTINPUT: {
                CodePoint wchars[17];
                strFromUtf8(wchars, countof(wchars), evt.text.text, nullptr);
                for (auto i = 0u; i < countof(wchars) && wchars[i] != 0; i++)
                    m_input_characters.push_back(wchars[i]);
                if (auto m = modal(); m != nullptr) {
                    m->handle_text_input();
                } else if (auto w = focus(); w != nullptr) {
                    w->handle_text_input();
                }
            } break;
            case SDL_MOUSEMOTION: {
                m_mouse = { evt.motion.x, evt.motion.y };
                handle_motion(evt.motion);
            } break;
            case SDL_MOUSEBUTTONDOWN: {
                handle_mousedown(evt.button);
            } break;
            case SDL_MOUSEBUTTONUP: {
                handle_click(evt.button);
            } break;
            case SDL_MOUSEWHEEL: {
                handle_wheel(evt.wheel);
            } break;
            default:
                break;
            }
        }

        render();

        auto end_render = std::chrono::steady_clock::now();
        static std::chrono::duration<double> one_frame(1.0/60.0);
        auto render_time = end_render - start_render;
        if (one_frame > render_time) {
            std::this_thread::sleep_for(one_frame - render_time);
        }
        end_render = std::chrono::steady_clock::now();
        m_last_render_time = end_render - start_render;
        start_render = end_render;
    }
}

bool dispatch_to(Widget* w, SDL_Keysym sym)
{
    if (w->dispatch(sym))
        return true;
    if (auto container = dynamic_cast<WidgetContainer*>(w); container != nullptr) {
        for (auto& c : container->components()) {
            if (dispatch_to(c, sym))
                break;
        }
    }
    return false;
};

bool App::dispatch(SDL_Keysym sym)
{
    m_last_key = sym;
    return Layout::dispatch(sym);
}

std::string App::input_buffer()
{
    std::string ret;
    for (auto const code_point : m_input_characters) {
        oassert(code_point < 256, "Unicode not supported yet");
        ret += (char)code_point;
    }
    m_input_characters.clear();
    return ret;
}

}
