/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <filesystem>
#include <sstream>
#include <string>

#include <SDL.h>

#include <core/Logging.h>

#include <Command.h>
#include <EditorState.h>
//#include <Scrollbar.h>
#include <Geometry.h>
#include <Key.h>
#include <Widget.h>

#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 800
#endif

#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 600
#endif

#ifndef WIDGET_BORDER_X
#define WIDGET_BORDER_X 8
#endif

#ifndef WIDGET_BORDER_Y
#define WIDGET_BORDER_Y 20
#endif

#ifndef SCROLL_BAR_SIZE
#define SCROLL_BAR_SIZE 8
#endif

#ifndef SCRATCH_EPSILON
#define SCRATCH_EPSILON 0.0001f
#endif /* SCRATCH_EPSILON */

#ifndef SCRATCH_UTF8_CHAR_FACTOR
#define SCRATCH_UTF8_CHAR_FACTOR 2
#endif /* SCRATCH_UTF8_CHAR_FACTOR */

#ifndef SCRATCH_MERGE_UNDO_REDO
#define SCRATCH_MERGE_UNDO_REDO 1
#endif /* SCRATCH_MERGE_UNDO_REDO */

#ifndef SCRATCH_CASE_FUNC
#define SCRATCH_CASE_FUNC ::tolower
#endif /* SCRATCH_CASE_FUNC */

#ifndef countof
#define countof(A) (sizeof(A) / sizeof(*(A)))
#endif /* countof */

static const int COLORIZE_DELAY_FRAME_COUNT = 60;

namespace Scratch {

extern_logging_category(scratch);

using Key=int;
using Keycode=int32_t;
static_assert(sizeof(Keycode) == sizeof(SDL_Keycode), "Wrong type size.");

class Messenger {
public:
    void message(char const* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        vmessage(msg, args);
        va_end(args);
    }

    virtual void vmessage(char const*, va_list) = 0;
};

class SDLContext;

class App : public Widget /*, public Messenger */ {
public:
    App(std::string, SDLContext*);
    static App& instance();

    ~App() override = default;
    void quit() { m_quit = true; }
    Editor* editor();

    [[nodiscard]] bool is_running() const { return !m_quit; }
    [[nodiscard]] SDLContext* context() { return m_context.get(); }
    [[nodiscard]] SDLContext const* context() const { return m_context.get(); }
    SDL_Renderer* renderer();

//    [[nodiscard]] bool handle(KeyCode) override;
//    void vmessage(char const*, va_list) override;
    std::vector<std::string> status() override;


    int width() const;
    int height() const;
    intptr_t active() const;
    void active(intptr_t);
    SDL_Color color(PaletteIndex color);

    Coordinates cursor_position() const { return { 1, 1 }; }

    void event_loop();
    void render() override;
    bool dispatch(SDL_Keysym) override;
    std::string input_buffer();
    void schedule(Command const* cmd);

    void add_component(Widget*);
    [[nodiscard]] virtual std::vector<Widget*> components();

    template <class ComponentClass>
    requires std::derived_from<ComponentClass, Widget>
    ComponentClass* get_component()
    {
        for (auto& c : components()) {
            if (auto casted = dynamic_cast<ComponentClass*>(c); casted != nullptr)
                return casted;
        }
        return nullptr;
    }

    void add_modal(Widget*);
    Widget* modal();
    void dismiss_modal();

private:
    void on_event(SDL_Event*);

    static App* s_app;

    std::string m_name;
    bool m_quit { false };
    int m_width { 0 };
    int m_height { 0 };
    intptr_t m_active { 0 };
    int m_mouse_clicked_count { 0 };

    Palette m_palette;
    InputBuffer m_input_characters;

    Vec2 m_widgetPos;
    Vec2 m_widgetSize;
//    bool m_widgetFocused = true;
//    bool m_widgetHoverable = true;

    Vec2 m_contentSize;
//    float m_scrollX = 0.0f;
    float m_scrollY = 0.0f;

    unsigned m_frameCount = 0;

    std::vector<std::unique_ptr<Widget>> m_components;
    std::vector<std::unique_ptr<Widget>> m_modals;
    std::unique_ptr<SDLContext> m_context;
    std::deque<Command const*> m_pending_commands;
    SDLKey m_last_key { SDLK_UNKNOWN, KMOD_NONE };
};

}
