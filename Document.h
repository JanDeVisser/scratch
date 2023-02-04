/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <deque>
#include <filesystem>

#include <SDL.h>

#include <lexer/BasicParser.h>

#include <Forward.h>
#include <Parser/CPlusPlus.h>

namespace Scratch {

namespace fs=std::filesystem;

using namespace Obelix;

struct FileType {
    std::vector<std::string> extensions;
    std::string mimetype;
    std::function<Parser::ScratchParser*()> parser_builder;
};

struct Line {
    Line() = default;
    explicit Line(std::string t)
        : text(std::move(t))
    {
    }

    std::string text;
    std::vector<Token> tokens {};
};

struct DocumentPosition {
    int line { 0 };
    int column { 0 };

    void clear()
    {
        line = column = 0;
    }
    auto operator<=>(DocumentPosition const& other) const = default;
};

class Document {
public:
    explicit Document(Editor *);

    [[nodiscard]] std::string const& line(size_t) const;
    [[nodiscard]] int line_length(size_t) const;
    [[nodiscard]] int line_count() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t parsed() const;
    [[nodiscard]] fs::path const& path() const;

    [[nodiscard]] int screen_top() const { return m_screen_top; }
    [[nodiscard]] int screen_left() const { return m_screen_left; }
    [[nodiscard]] int rows() const;
    [[nodiscard]] int columns() const;
    [[nodiscard]] DocumentPosition const& point() const { return m_point; }
    [[nodiscard]] DocumentPosition const& mark() const { return m_mark; }
    [[nodiscard]] int point_line() const { return m_point.line; }
    [[nodiscard]] int point_column() const { return m_point.column; }
    [[nodiscard]] int virtual_point_column() const { return m_virtual_point_column; }

    void split_line();
    void insert(std::string const&);
    void join_lines();

    void reset_selection();
    void extend_selection(int);
    std::string selected_text();
    void erase_selection();
    void copy_to_clipboard();
    void cut_to_clipboard();
    void paste_from_clipboard();

    void move_to(int, int, bool);
    void up(bool);
    void down(bool);
    void left(bool);
    void word_left(bool);
    void right(bool);
    void word_right(bool);
    void page_up(bool);
    void page_down(bool);
    void home(bool);
    void end(bool);

    void clear();
    std::string load(std::string const&);
    std::string save();
    std::string save_as(std::string const&);
    [[nodiscard]] bool dirty() const { return m_dirty; }

    void render();
    bool dispatch(SDL_Keysym);
    void handle_click(int, int);
    void handle_text_input();

    Token lex();
    void rewind();
    void invalidate();
    auto last_parse_time() const { return m_last_parse_time.count(); }

private:
    void assign_to_parser();
    void update_internals_after_move(bool);

    Editor* m_editor;
    fs::path m_path {};
    bool m_dirty { false };
    bool m_cleared { true };
    FileType m_filetype;
    std::unique_ptr<Parser::ScratchParser> m_parser;

    std::vector<Line> m_lines {};

    int m_screen_top {0};
    int m_screen_left {0};
    DocumentPosition m_point;
    DocumentPosition m_mark;
    size_t m_virtual_point_column {0};
    std::chrono::milliseconds m_last_parse_time { 0 };
};

}
