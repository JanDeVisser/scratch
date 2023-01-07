/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include <SDL.h>

#include <core/Logging.h>

#include <EditorState.h>
//#include <Scrollbar.h>
#include <Geometry.h>
#include <Widget.h>

#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 640
#endif

#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 480
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

    [[nodiscard]] int rows() const;
    [[nodiscard]] int columns() const;
//    [[nodiscard]] bool handle(KeyCode) override;
//    void vmessage(char const*, va_list) override;


    int width() const;
    int height() const;
    intptr_t active() const;
    void active(intptr_t);
    SDL_Color color(PaletteIndex color);

    Coordinates cursor_position() const { return { 1, 1 }; }

    void event_loop();
    void render() override;
//    void dispatch();
    void onEvent(SDL_Event*);

    void add_component(Widget*);
    [[nodiscard]] virtual std::vector<std::unique_ptr<Widget>> const& components() { return m_components; }

    template <class ComponentClass>
    requires std::derived_from<ComponentClass, Widget>
    ComponentClass* get_component()
    {
        for (auto& c : components()) {
            if (auto casted = dynamic_cast<ComponentClass*>(c.get()); casted != nullptr)
                return casted;
        }
        return nullptr;
    }

private:

    static App* s_app;

    std::string m_name;
    bool m_quit { false };
    int m_width { 0 };
    int m_height { 0 };
    intptr_t m_active { 0 };
    int m_mouse_clicked_count { 0 };

    Palette m_palette;
    InputBuffer m_inputCharacters;

    Vec2 m_widgetPos;
    Vec2 m_widgetSize;
    bool m_widgetFocused = true;
    bool m_widgetHoverable = true;

    Vec2 m_contentSize;
    float m_scrollX = 0.0f;
    float m_scrollY = 0.0f;

    unsigned m_frameCount = 0;

    std::vector<std::unique_ptr<Widget>> m_components;
    std::unique_ptr<SDLContext> m_context;
};

}
