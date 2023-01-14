/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <array>
#include <cstdio>
#include <sstream>

#include <SDL2_gfxPrimitives.h>

#include <App.h>
#include <Editor.h>
#include <Forward.h>
#include <MessageBar.h>
#include <SDLContext.h>
#include <StatusBar.h>
#include <Text.h>

namespace Scratch {

App* App::s_app { nullptr };

App& App::instance()
{
    oassert(s_app != nullptr, "No App instantiated");
    return *s_app;
}

App::App(std::string name, SDLContext* ctx)
    : m_name(std::move(name))
    , m_palette(DarkPalette())
    , m_context(ctx)
{
    oassert(s_app == nullptr, "App is a singleton");
    s_app = this;
    Widget::char_width = m_context->character_width();
    Widget::char_height = m_context->character_height();
}

void App::add_component(Widget* widget)
{
    m_components.emplace_back(widget);
}

std::vector<Widget*> App::components()
{
    std::vector<Widget*> ret;
    for (auto const& c : m_components) {
        ret.push_back(c.get());
    }
    return ret;
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

Editor* App::editor()
{
    return get_component<Editor>();
};

SDL_Renderer* App::renderer()
{
    return m_context->renderer();
}

void App::render()
{
    m_frameCount++;
    m_mouse_clicked_count = 0;

    for (auto const& c : components()) {
        c->render();
    }
    if (m_modals.empty()) {
        if (!m_pending_commands.empty()) {
            auto cmd = m_pending_commands.front();
            add_modal(new CommandHandler(*cmd));
            m_pending_commands.pop_front();
        }
    } else {
        for (auto const& m : m_modals) {
            m->render();
        }
    }
}

void App::schedule(Command const* cmd)
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
    uint32_t c = m_palette[(size_t)color];
    return *((SDL_Color*)&c);
}

void App::event_loop()
{
    Uint64 timestamp = 0;
    SDL_Event e;
    while (!m_quit) {
        // Processes events.
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: {
                m_quit = true;
            } break;
            default:
                break;
            }
            on_event(&e);
        }

        // Renders.
        SDL_SetRenderDrawColor(renderer(), 0x2e, 0x32, 0x38, 0xff);
        SDL_RenderClear(renderer());

        render();

        const Uint64 now = SDL_GetPerformanceCounter();
        if (timestamp == 0)
            timestamp = now;
        const Uint64 diff = now - timestamp;
        const double ddiff = (double)diff / SDL_GetPerformanceFrequency();
        const double rest = 1.0 / 60.0 - ddiff; // 60 FPS.
        timestamp = now;
        if (rest > 0)
            SDL_Delay((Uint32)(rest * 1000));

        SDL_RenderPresent(renderer());
    }
}

void App::on_event(SDL_Event* evt)
{
    switch (evt->type) {
    case SDL_WINDOWEVENT: {
        switch (evt->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_RESIZED: {
            m_widgetPos = Vec2(WIDGET_BORDER_X, WIDGET_BORDER_Y);
            SDL_GetRendererOutputSize(renderer(), &m_width, &m_height);
            m_widgetSize = Vec2((float)m_width - WIDGET_BORDER_X * 2 - 2 - SCROLL_BAR_SIZE,
                (float)m_height - WIDGET_BORDER_Y * 2 - 2 - SCROLL_BAR_SIZE);
        } break;
        }
    } break;
    case SDL_KEYDOWN: {
        if (auto m = modal(); m != nullptr) {
            m->dispatch(evt->key.keysym);
        } else if (!dispatch(evt->key.keysym)) {
            for (auto& c : components()) {
                if (c->dispatch(evt->key.keysym))
                    break;
            }
        }
    } break;
    case SDL_TEXTINPUT: {
        CodePoint wchars[17];
        strFromUtf8(wchars, countof(wchars), evt->text.text, nullptr);
        for (int i = 0; i < countof(wchars) && wchars[i] != 0; i++)
            m_input_characters.push_back(wchars[i]);
        if (auto m = modal(); m != nullptr) {
            m->handle_text_input();
        } else {
            handle_text_input();
            for (auto& c : components()) {
                c->handle_text_input();
            }
        }
    } break;
    case SDL_MOUSEBUTTONUP: {
        m_mouse_clicked_count = evt->button.clicks;
    } break;
    case SDL_MOUSEWHEEL: {
        if (evt->wheel.y < 0)
            m_scrollY += 32.0f;
        else if (evt->wheel.y > 0)
            m_scrollY -= 32.0f;
        m_scrollY = clamp(m_scrollY,
            0.0f,
            m_contentSize.y() <= m_widgetSize.y() ? 0.0f : m_contentSize.y() - m_widgetSize.y());
    } break;
    default:
        break;
    }
}

bool App::dispatch(SDL_Keysym sym)
{
    m_last_key = sym;
    if (auto* cmd = Command::command_for_key(m_last_key); cmd != nullptr) {
        schedule(cmd);
        return true;
    }
    return false;
}

std::vector<std::string> App::status()
{
    return { m_last_key.to_string() };
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
