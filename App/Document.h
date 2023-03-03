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

#include <App/Forward.h>
#include <Commands/Command.h>
#include <Parser/CPlusPlus.h>
#include <Widget/Widget.h>

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

    int start_index {0};
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

enum class EditActionType {
    InsertText,
    DeleteText,
    CursorMove,
};

struct DocumentCommands : public Commands {
    DocumentCommands();
};

class EditAction {
public:
    static EditAction insert_text(int, std::string);
    static EditAction delete_text(int, std::string);
    static EditAction move_cursor(int, int);

    [[nodiscard]] int cursor() const { return m_cursor; }
    [[nodiscard]] EditActionType type() const { return m_type; }
    [[nodiscard]] std::string const& text() const { return m_text; }
    [[nodiscard]] int pointer() const { return m_pointer; }

    void undo(Document&) const;
    void redo(Document&) const;
    std::optional<EditAction> merge(EditAction const&) const;
private:
    EditAction(EditActionType, int, std::string);
    EditAction(EditActionType, int, int);
    EditActionType m_type;
    int m_cursor {0};
    int m_pointer {0};
    std::string m_text;
};

class Document : public Widget {
public:
    explicit Document(Editor *);

    [[nodiscard]] int text_length() const;
    [[nodiscard]] std::string line(size_t) const;
    [[nodiscard]] std::string const& text() const { return m_text; }
    [[nodiscard]] int line_length(size_t) const;
    [[nodiscard]] int line_count() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t parsed() const;
    [[nodiscard]] fs::path const& path() const;

    [[nodiscard]] int screen_top() const { return m_screen_top; }
    [[nodiscard]] int screen_left() const { return m_screen_left; }
    [[nodiscard]] int rows() const;
    [[nodiscard]] int columns() const;

    [[nodiscard]] int find_line_number(int) const;
    [[nodiscard]] DocumentPosition position(int) const;
    [[nodiscard]] int point() const { return m_point; };
    [[nodiscard]] int mark() const { return m_mark; }
    [[nodiscard]] int point_line() const;
    [[nodiscard]] int point_column() const;
    [[nodiscard]] int mark_line() const;
    [[nodiscard]] int mark_column() const;

    void undo();
    void redo();

    void split_line();
    void insert(std::string const&);
    void join_lines();
    void duplicate_line();

    enum class TransposeDirection {
        Up,
        Down,
    };
    void transpose_lines(TransposeDirection = TransposeDirection::Down);

    void reset_selection();
    void extend_selection(int);
    void select_word();
    void select_line();
    void select_all();
    std::string selected_text();
    void erase_selection();
    void copy_to_clipboard();
    void cut_to_clipboard();
    void paste_from_clipboard();

    void set_point_and_mark(int, int = -1);
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

    bool find(std::string const&);
    bool find_next();

    void clear();
    std::string load(std::string const&);
    std::string save();
    std::string save_as(std::string const&);
    [[nodiscard]] bool dirty() const { return m_dirty; }

    void render() override;
    bool dispatch(SDL_Keysym) override;
    void mousedown(int, int);
    void motion(int, int);
    void click(int, int, int);
    void wheel(int);
    void handle_text_input() override;

    Token const& lex();
    void rewind();
    void invalidate();
    [[nodiscard]] auto last_parse_time() const { return m_last_parse_time.count(); }

    std::optional<ScheduledCommand> command(std::string const&) const override;
    [[nodiscard]] std::vector<Command> commands() const override;

private:
    void insert_text(std::string const&, int = -1);
    void erase(int, int);
    void move_point(int);
    void update_internals(bool, int = -1);
    void add_edit_action(EditAction);

    Editor* m_editor;
    fs::path m_path {};
    bool m_dirty { false };
    FileType m_filetype;
    std::unique_ptr<Parser::ScratchParser> m_parser;

    std::string m_text;
    bool m_changed { false };
    std::vector<Line> m_lines {};

    int m_screen_top {0};
    int m_screen_left {0};
    int m_point {0};
    int m_mark {0};
    std::string m_find_term;
    bool m_found { true };
    std::vector<EditAction> m_edits;
    int m_undo_pointer { -1 };
    std::chrono::milliseconds m_last_parse_time { 0 };
    static DocumentCommands s_document_commands;

    friend EditAction;
};

}
