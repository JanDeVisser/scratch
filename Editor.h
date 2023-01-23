/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <SDL.h>

#include <Document.h>
#include <EditorState.h>
#include <Widget.h>

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

class Editor : public WindowedWidget {
public:
    Editor();

    [[nodiscard]] Document* document() { return m_current_document; }
    void new_buffer();
    std::string open_file(fs::path const&);
    std::string save_file();
    std::string save_file_as(fs::path const&);
    std::string save_all();
    void move_to(int line, int column);
    void switch_to(std::string const&);
    std::vector<std::string> status() override;

    [[nodiscard]] int rows() const;
    [[nodiscard]] int columns() const;
    [[nodiscard]] int line_top(int line) const;
    [[nodiscard]] int line_bottom(int line) const;
    [[nodiscard]] int column_left(int column) const;
    [[nodiscard]] int column_right(int column) const;
    [[nodiscard]] static int line_height();
    [[nodiscard]] static int column_width();

    void render() override;
    void text_cursor(int line, int column);
    void mark_current_line(int line);
    bool dispatch(SDL_Keysym) override;
    void handle_text_input() override;
    void append(DisplayToken const&);
    void newline();

    [[nodiscard]] std::vector<Document*> documents() const;
    [[nodiscard]] Document* document(fs::path const&) const;

private:
    std::vector<std::unique_ptr<Document>> m_documents {};
    Document* m_current_document { nullptr };
    int m_line { 0 };
    int m_column { 0 };
    int m_rows { -1 };
    int m_columns { -1 };
};

}
