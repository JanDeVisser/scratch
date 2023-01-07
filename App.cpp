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

enum class CommandParameterType {
    String,
    Integer,
    Filename,
    ExistingFilename,
    Directory,
    ExistingDirectory,
};

struct CommandParameter {
    std::string prompt;
    CommandParameterType type;
};

struct Command {
    std::string synopsis;
    std::vector<CommandParameter> parameters;
    std::function<void(strings const&)> function;
};

/*
static std::map<std::string, Command> s_commands = {
    { "scratch-quit", { "Quits the editor", {}, [](strings const&) -> void { App::instance().quit(); } } },
    { "open-file", { "Open file", { { "File to open", CommandParameterType::ExistingFilename } }, [](strings const& args) -> void {
                        App::instance().get_component<Editor>()->open_file(args[0]);
                    } } },
    { "save-file", { "Save current file", {}, [](strings const&) -> void { App::instance().get_component<Editor>()->save_file(); } } },
};

static std::map<KeyCode, std::string> s_key_bindings = {
    { CTRL_Q, "scratch-quit" },
    { KEY_F2, "open-file" },
    { KEY_F3, "save-file" },
};
 */

struct Clipper {
public:
    Clipper(SDL_Renderer* rnd, const SDL_Rect& rect)
        : m_renderer(rnd)
    {
        SDL_RenderGetClipRect(m_renderer, &m_clip);
        SDL_RenderSetClipRect(m_renderer, &rect);
    }

    ~Clipper()
    {
        if (m_clip.w == 0 || m_clip.h == 0)
            SDL_RenderSetClipRect(m_renderer, nullptr);
        else
            SDL_RenderSetClipRect(m_renderer, &m_clip);
    }

private:
    SDL_Renderer* m_renderer = nullptr;
    SDL_Rect m_clip;
};

App* App::s_app { nullptr };

App& App::instance()
{
    oassert(s_app != nullptr, "No App instantiated");
    return *s_app;
}

App::App(std::string name, SDLContext *ctx)
    : m_name(std::move(name))
    , m_palette(DarkPalette())
    , m_context(ctx)
{
    oassert(s_app == nullptr, "App is a singleton");
    s_app = this;
}

#ifdef SCRATCH_CONSOLE

void App::event_loop()
{
    while (!m_quit) {
        render_app();
        auto key_maybe = Display::getkey();
        if (key_maybe.is_error()) {
            if (key_maybe.error() == EINTR) {
                auto resize_maybe = resize();
                if (resize_maybe.is_error()) {
                    fprintf(stderr, "ERROR: %s\n", resize_maybe.error().c_str());
                    break;
                }
                continue;
            }
            fprintf(stderr, "ERROR: something went wrong during reading of the user input: %s\n", strerror(errno));
            break;
        }
        dispatch(key_maybe.value());
    }
    printf("\033[2J\033[H");
}

void App::render_app()
{
    display()->clear();
    for (auto const& c : components()) {
        c->pre_render();
    }
    for (auto const& c : components()) {
        c->render();
    }
    for (auto iter = components().rbegin(); iter != components().rend(); ++iter) {
        (*iter)->post_render();
    }
    display()->render();
}

void App::dispatch(KeyCode key)
{
    if (!handle(key)) {
        for (auto const& c : components()) {
            if (c->handle(key))
                break;
        }
    }
}

bool App::handle(KeyCode key)
{
    if (s_key_bindings.contains(key)) {
        auto cmd = s_key_bindings.at(key);
        assert(s_commands.contains(cmd));
        auto cmd_descr = s_commands.at(cmd);
        cmd_descr.function(strings {});
        return true;
    }
    return false;
}

ErrorOr<void, std::string> App::resize()
{
    return {};
}

#endif

void App::add_component(Widget *widget)
{
    m_components.emplace_back(widget);
}

Editor* App::editor()
{
    return get_component<Editor>();
};

SDL_Renderer * App::renderer()
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
}

int App::width() const
{
    return m_width;
}

int App::height() const
{
    return m_height;
}

int App::rows() const
{
    return height() / context()->character_width();
}


int App::columns() const
{
    return m_height / context()->character_height();
}

intptr_t App::active() const
{
    return m_active;
}

void App::active(intptr_t val)
{
    m_active = val;
}

SDL_Color App::color(PaletteIndex color)
{
    auto c = m_palette[(size_t) color];
    uint8_t r = (c >> 24) & 0xff;
    uint8_t g = (c >> 16) & 0xff;
    uint8_t b = (c >> 8) & 0xff;
    uint8_t a = c & 0xff;
    return { r, g, b, a };
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
            onEvent(&e);
        }

        // Renders.
        SDL_SetRenderDrawColor(renderer(), 0x2e, 0x32, 0x38, 0xff);
        SDL_RenderClear(renderer());

        boxColor(
            renderer(),
            WIDGET_BORDER_X, WIDGET_BORDER_Y,
            // Reserves space for scroll bars.
            width() - WIDGET_BORDER_X - 2 - SCROLL_BAR_SIZE, height() - WIDGET_BORDER_Y - 2 - SCROLL_BAR_SIZE,
            0xff2c2c2c);

        render(); // Renders the widget and processes events.

        stringColor(renderer(), WIDGET_BORDER_X, (WIDGET_BORDER_Y - 8) / 2, "Syntax highlighting code edit widget", 0xffffffff);
        Coordinates cp = cursor_position();
        std::stringstream ss;
        ss << "Ln " << cp.line + 1 << ", Col " << cp.column + 1;
        stringColor(renderer(), WIDGET_BORDER_X, height() - WIDGET_BORDER_Y + (WIDGET_BORDER_Y - 8) / 2, ss.str().c_str(), 0xffffffff);

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

void App::onEvent(SDL_Event* evt)
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
    case SDL_TEXTINPUT: {
        CodePoint wchars[17];
        strFromUtf8(wchars, countof(wchars), evt->text.text, nullptr);
        for (int i = 0; i < countof(wchars) && wchars[i] != 0; i++)
            m_inputCharacters.push_back(wchars[i]);
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

}
