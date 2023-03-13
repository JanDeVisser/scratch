/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <SDL.h>

#include <App/Document.h>
#include <App/EditorState.h>
#include <Widget/Widget.h>

namespace Scratch {

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

class EditorCommands : public Commands {
public:
    EditorCommands();
};

struct BufferId {
    size_t index;
    std::string title;
    std::string short_title;
};

class Editor : public WindowedWidget {
public:
    Editor();

    [[nodiscard]] Buffer* buffer() const;
    [[nodiscard]] Document* document() const;
    void switch_to(std::string const&);
    void switch_to(size_t);
    [[nodiscard]] std::vector<Buffer*> buffers() const;
    [[nodiscard]] std::vector<BufferId> buffer_ids() const;
    [[nodiscard]] std::vector<Document*> documents() const;
    [[nodiscard]] Buffer* buffer(std::string const&) const;
    [[nodiscard]] Buffer* buffer(size_t) const;
    [[nodiscard]] Document* document(fs::path const&) const;

    void new_file();
    std::string open_file(fs::path const&);
    std::string save_all() const;

    [[nodiscard]] int rows() const;
    [[nodiscard]] int columns() const;
    [[nodiscard]] int line_top(int line) const;
    [[nodiscard]] int line_bottom(int line) const;
    [[nodiscard]] static int column_left(int column);
    [[nodiscard]] static int column_right(int column);
    [[nodiscard]] int line_height() const;
    [[nodiscard]] static int column_width();

    void resize(Box const&) override;
    void render() override;
    void text_cursor(int line, int column);
    void mark_current_line(int line);
    bool dispatch(SDL_Keysym) override;
    void handle_mousedown(SDL_MouseButtonEvent const& event) override;
    void handle_motion(SDL_MouseMotionEvent const& event) override;
    void handle_click(SDL_MouseButtonEvent const& event) override;
    void handle_wheel(SDL_MouseWheelEvent const& event) override;
    void handle_text_input() override;
    std::optional<ScheduledCommand> command(std::string const&) const override;
    [[nodiscard]] std::vector<ScheduledCommand> commands() const override;

    void append(DisplayToken const&);
    void newline();

private:
    std::vector<std::unique_ptr<Buffer>> m_buffers {};
    Buffer* m_current_buffer { nullptr };
    int m_line { 0 };
    int m_column { 0 };
    int m_rows { -1 };
    int m_columns { -1 };
    int m_line_height { 0 };
    std::optional<Position> m_mouse_down_at;
    static EditorCommands s_editor_commands;
};

}
