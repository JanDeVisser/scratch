/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>
#include <fstream>

#include <core/Format.h>

#include <App/Document.h>
#include <App/Editor.h>
#include <App/Scratch.h>
#include <Parser/CPlusPlus.h>
#include <Parser/PlainText.h>
#include <Scribble/Scribble.h>
#include <Widget/App.h>

using namespace Obelix;
using namespace Scratch::Parser;
using namespace Scratch::Scribble;

namespace Scratch {

FileType s_filetypes[] = {
    // Plain Text parser must be in slot 0! Do not sort down!
    { { ".txt" }, "text/plain", []() -> ScratchParser* {
         return new PlainTextParser();
     } },
    { { ".cpp", ".h", ".hpp" }, "text/x-cpp", []() -> ScratchParser* {
         return new CPlusPlusParser();
     } },
    { { ".scratch" }, "text/x-scratch", []() -> ScratchParser* {
         return new class Scribble();
     } },
};

FileType const& get_filetype(fs::path const& file)
{
    auto ext = file.extension();
    if (ext.empty())
        return s_filetypes[0];
    for (auto const& type : s_filetypes) {
        for (auto const& e : type.extensions) {
            if (e == ext)
                return type;
        }
    }
    return s_filetypes[0];
}

DocumentCommands::DocumentCommands()
{
    register_command(
        { "copy-to-clipboard", "Copy selection to clipboard", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.copy_to_clipboard();
            } },
        { SDLK_c, KMOD_CTRL });
    register_command(
        { "cut-to-clipboard", "Cut selection to clipboard", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.cut_to_clipboard();
            } },
        { SDLK_x, KMOD_CTRL });
    register_command(
        { "duplicate-line", "Duplicate current line", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.duplicate_line();
            } },
        { SDLK_d, KMOD_CTRL });
    register_command(
        { "find-first", "Find",
            { { "Find", CommandParameterType::String,
                []() -> std::string {
                    auto* doc = Scratch::editor()->document();
                    if (doc == nullptr)
                        return "";
                    return doc->selected_text();
                } } },
            [](Widget& w, strings const& args) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.find(args[0]);
            } },
        { SDLK_f, KMOD_CTRL });
    register_command(
        { "find-next", "Find next", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.find_next();
            } },
        { SDLK_f, KMOD_CTRL | KMOD_SHIFT });
    register_command(
        { "goto-line-column", "Goto line:column",
            { { "Line:Column to go to", CommandParameterType::String } },
            [](Widget& w, strings const& args) -> void {
                auto line_col = split(args[0], ':');
                if (line_col.size() < 1)
                    return;
                if (auto line_maybe = try_to_long<std::string>()(line_col[0]); line_maybe.has_value()) {
                    int col = -1;
                    if (line_col.size() > 1) {
                        if (auto col_maybe = try_to_long<std::string>()(line_col[1]); col_maybe.has_value())
                            col = col_maybe.value();
                    }
                    auto& doc = dynamic_cast<Document&>(w);
                    doc.move_to(line_maybe.value() - 1, col - 1, false);
                }
            } },
        { SDLK_g, KMOD_CTRL });
    register_command(
        { "paste-from-clipboard", "Paste text from clipboard", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.paste_from_clipboard();
            } },
        { SDLK_v, KMOD_CTRL });
    register_command(
        { "redo", "Redo edit", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.redo();
            } },
        { SDLK_y, KMOD_CTRL });
    register_command(
        { "save-current-as", "Save current file as",
            { { "New file name", CommandParameterType::String } },
            [](Widget& w, strings const& args) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.save_as(args[0]);
            } },
        { SDLK_s, KMOD_CTRL | KMOD_GUI });
    register_command(
        { "save-file", "Save current file", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                if (doc.path().empty()) {
                    App::instance().schedule(doc.command("save-current-as").value());
                } else {
                    doc.save();
                }
            } },
        { SDLK_s, KMOD_CTRL });
    register_command(
        { "select-all", "Select all", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.select_all();
            } },
        { SDLK_a, KMOD_CTRL });
    register_command(
        { "select-line", "Select line", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.select_line();
            } });
    register_command(
        { "select-word", "Select word", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.select_word();
            } },
        { SDLK_UP, KMOD_GUI });
    register_command(
        { "transpose-lines-down", "Transpose lines down", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.transpose_lines(Document::TransposeDirection::Down);
            } },
        { SDLK_DOWN, KMOD_ALT });
    register_command(
        { "transpose-lines-up", "Transpose lines up", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.transpose_lines(Document::TransposeDirection::Up);
            } },
        { SDLK_UP, KMOD_ALT });
    register_command(
        { "undo", "Undo edit", {},
            [](Widget& w, strings const&) -> void {
                auto& doc = dynamic_cast<Document&>(w);
                doc.undo();
            } },
        { SDLK_z, KMOD_CTRL });
}

DocumentCommands Document::s_document_commands;

Document::Document(Editor* editor)
    : Buffer(editor)
{
    m_path.clear();
    m_lines.emplace_back();
    m_filetype = get_filetype("");
    m_parser = std::unique_ptr<ScratchParser>(m_filetype.parser_builder());
    m_commands = &s_document_commands;
}

fs::path const& Document::path() const
{
    return m_path;
}

std::string Document::title() const
{
    return m_path;
}

std::string Document::short_title() const
{
    return fs::relative(path()).string();
}

std::string Document::status() const
{
    std::stringstream ss;
    ss << point_line() + 1 << ":" << point_column() + 1 << " [";
    ss << screen_top() + 1 << ":" << screen_left() + 1 << "]";
    return ss.str();
}

std::string Document::line(size_t line_no) const
{
    assert(line_no < m_lines.size());
    if (line_no == m_lines.size() - 1)
        return m_text.substr(m_lines[line_no].start_index);
    return m_text.substr(m_lines[line_no].start_index, m_lines[line_no + 1].start_index);
}

int Document::text_length() const
{
    return static_cast<int>(m_text.length());
}

int Document::line_length(size_t line_no) const
{
    assert(line_no < m_lines.size());
    if (line_no == m_lines.size() - 1)
        return text_length() - m_lines[line_no].start_index;
    return m_lines[line_no + 1].start_index - m_lines[line_no].start_index - 1;
}

int Document::line_count() const
{
    return static_cast<int>(m_lines.size());
}

bool Document::empty() const
{
    return m_lines.empty() || (m_lines.size() == 1 && m_lines[0].tokens.empty());
}

size_t Document::parsed() const
{
    return !m_parser->tokens().empty();
}

void Document::split_line()
{
    insert("\n");
}

void Document::join_lines()
{
    int ix;
    for (ix = m_point; (ix > 0) && (m_text[ix] != '\n'); --ix)
        ;
    if (ix > 0) {
        add_edit_action(EditAction::delete_text(ix, m_text.substr(ix, 1)));
        m_text.erase(ix, 1);
        m_point = ix - 1;
        update_internals(false);
    }
}

void Document::duplicate_line()
{
    auto point = m_point;
    auto line = find_line_number(m_point);
    auto len = line_length(line);
    m_mark = m_point = m_lines[line].start_index + len;
    insert("\n" + m_text.substr(m_lines[line].start_index, len));
    move_point(point + len + 1);
    m_mark = m_point;
    update_internals(false);
}

void Document::transpose_lines(TransposeDirection direction)
{
    if (line_count() < 2)
        return;
    auto line = find_line_number(m_point);
    auto column = m_point - m_lines[line].start_index;
    if (((direction == TransposeDirection::Down) && (line < line_count() - 1)) || ((direction == TransposeDirection::Up) && (line > 0))) {
        auto top_line = (direction == TransposeDirection::Down) ? line : line - 1;
        auto bottom_line = (direction == TransposeDirection::Down) ? line + 1 : line;
        auto top = m_text.substr(m_lines[top_line].start_index, line_length(top_line));
        auto bottom = m_text.substr(m_lines[bottom_line].start_index, line_length(bottom_line));
        auto offset = (direction == TransposeDirection::Down) ? line_length(top_line) + 1 + column : column;
        m_mark = m_lines[top_line].start_index;
        m_point = m_lines[bottom_line].start_index + line_length(bottom_line);
        add_edit_action(EditAction::delete_text(m_mark, top + "\n" + bottom));
        add_edit_action(EditAction::insert_text(m_mark, bottom + "\n" + top));
        erase(m_mark, m_point - m_mark);
        insert_text(bottom + "\n" + top);
        move_point(m_lines[top_line].start_index + offset);
        update_internals(false);
    }
}

void Document::insert_text(std::string const& str, int point)
{
    if (str.empty())
        return;
    if (point < 0)
        point = m_point;
    m_text.insert(point, str);
    m_changed = true;
    m_point += static_cast<int>(str.length());
}

void Document::insert(std::string const& str)
{
    if (str.empty())
        return;
    erase_selection();
    insert_text(str);
    add_edit_action(EditAction::insert_text(m_point - static_cast<int>(str.length()), str));
    update_internals(false);
}

void Document::reset_selection()
{
    m_mark = m_point;
}

void Document::extend_selection(int num)
{
    auto point = m_point;
    auto& left = (m_mark <= m_point) ? m_mark : m_point;
    auto& right = (m_mark <= m_point) ? m_point : m_mark;

    if (num < 0) {
        left += num;
        if (left < 0)
            left = 0;
    } else {
        right += num;
        if (right > static_cast<int>(m_text.length()))
            right = text_length();
    }
    add_edit_action(EditAction::move_cursor(point, m_point));
}

void Document::select_word()
{
    auto point = m_point;
    m_mark = m_point;
    if (isalnum(m_text[m_point]) || m_text[m_point] == '_') {
        while (m_point > 0 && (isalnum(m_text[m_point - 1]) || m_text[m_point - 1] == '_'))
            --m_point;
        while (m_mark < static_cast<int>(m_text.length()) && (isalnum(m_text[m_mark]) || m_text[m_mark] == '_'))
            ++m_mark;
    } else {
        while (m_point > 0 && !isalnum(m_text[m_point - 1]) && m_text[m_point - 1] != '_')
            --m_point;
        while (m_mark < text_length() && !isalnum(m_text[m_mark]) && m_text[m_mark] != '_')
            ++m_mark;
    }
    add_edit_action(EditAction::move_cursor(point, m_point));
}

void Document::select_line()
{
    auto line = static_cast<size_t>(find_line_number(m_point));
    move_point(m_lines[line].start_index);
    if (line < m_lines.size() - 1)
        m_mark = m_lines[line + 1].start_index;
    else
        m_mark = text_length();
}

void Document::select_all()
{
    m_mark = 0;
    move_point(text_length());
}

std::string Document::selected_text()
{
    if (m_point == m_mark)
        return "";
    int start_selection = std::min(m_point, m_mark);
    int end_selection = std::max(m_point, m_mark);
    return m_text.substr(start_selection, end_selection - start_selection);
}

void Document::erase(int point, int len)
{
    m_text.erase(point, len);
    m_mark = m_point = point;
    m_changed = true;
}

void Document::erase_selection()
{
    if (m_point == m_mark)
        return;
    int start_selection = std::min(m_point, m_mark);
    int end_selection = std::max(m_point, m_mark);
    add_edit_action(EditAction::delete_text(start_selection, m_text.substr(start_selection, end_selection - start_selection)));
    erase(start_selection, end_selection - start_selection);
    move_point(start_selection);
    update_internals(false);
}

void Document::copy_to_clipboard()
{
    if (m_point == m_mark) {
        home(false);
        end(true);
    }
    auto selection = selected_text();
    if (!selection.empty()) {
        if (auto err = SDL_SetClipboardText(selection.c_str()); err != 0) {
            fatal("Error copying selection to clipboard: {}", SDL_GetError());
        }
    }
}

void Document::cut_to_clipboard()
{
    copy_to_clipboard();
    erase_selection();
}

void Document::paste_from_clipboard()
{
    if (SDL_HasClipboardText() == SDL_FALSE)
        return;
    if (auto clipboard = SDL_GetClipboardText(); (clipboard != nullptr) && clipboard[0])
        insert(clipboard);
}

int Document::find_line_number(int cursor) const
{
    int line_min = 0;
    int line_max = line_count() - 1;
    while (true) {
        int line = line_min + (line_max - line_min) / 2;
        if ((line < line_count() - 1 && m_lines[line].start_index <= cursor && cursor < m_lines[line + 1].start_index) || (line == line_count() - 1 && m_lines[line].start_index <= cursor)) {
            return line;
        } else {
            if (m_lines[line].start_index > cursor) {
                line_max = line;
            } else {
                line_min = line + 1;
            }
        }
    }
}

DocumentPosition Document::position(int cursor) const
{
    auto line = find_line_number(cursor);
    return { line, cursor - m_lines[line].start_index };
}

int Document::point_line() const
{
    return find_line_number(m_point);
}

int Document::point_column() const
{
    auto pos = position(m_point);
    return pos.column;
}

int Document::mark_line() const
{
    return find_line_number(m_mark);
}

int Document::mark_column() const
{
    auto pos = position(m_mark);
    return pos.column;
}

void Document::set_point_and_mark(int point, int mark)
{
    if (mark < 0)
        mark = point;
    m_point = point;
    m_mark = mark;
    auto line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    if (m_screen_top > line || m_screen_top + rows() < line)
        m_screen_top = line - rows() / 2;
    if (m_screen_left > column || m_screen_left + columns() < column)
        m_screen_left = column - columns() / 2;
    update_internals(mark != point, line);
}

void Document::move_to(int line, int column, bool select)
{
    line = clamp(line, 0, (int)line_count() - 1);
    column = clamp(column, 0, (int)line_length(line));
    move_point(m_lines[line].start_index + column);
    if (m_screen_top > line || m_screen_top + rows() < line)
        m_screen_top = line - rows() / 2;
    if (m_screen_left > column || m_screen_left + columns() < column)
        m_screen_left = column - columns() / 2;
    update_internals(select, line);
}

void Document::update_internals(bool select, int line)
{
    if (line < 0)
        line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    m_screen_top = clamp(m_screen_top, std::max(0, line - editor()->rows() + 1), line);
    m_screen_left = clamp(m_screen_left, std::max(0, column - editor()->columns() + 1), column);
    if (!select)
        m_mark = m_point;
    if (m_changed) {
        m_parser->assign(m_text);
        m_lines.clear();
        m_lines.emplace_back();
        m_parser->invalidate();
    }
}

void Document::move_point(int point)
{
    if (point == m_point)
        return;
    add_edit_action(EditAction::move_cursor(m_point, point));
    m_point = point;
}

void Document::add_edit_action(EditAction action)
{
    if (m_undo_pointer < static_cast<int>(m_edits.size()) - 1) {
        m_edits.erase(m_edits.begin() + clamp(m_undo_pointer, 0, static_cast<int>(m_edits.size()) - 1), m_edits.end());
    }
    if (!m_edits.empty()) {
        if (auto merged = m_edits.back().merge(action); merged.has_value()) {
            m_edits.pop_back();
            m_edits.push_back(merged.value());
            m_undo_pointer = static_cast<int>(m_edits.size() - 1);
            return;
        }
    }
    m_edits.push_back(std::move(action));
    m_undo_pointer = static_cast<int>(m_edits.size() - 1);
}

void Document::undo()
{
    if (!m_edits.empty() && (m_undo_pointer >= 0) && (m_undo_pointer < static_cast<int>(m_edits.size()))) {
        m_edits[m_undo_pointer--].undo(*this);
    }
}

void Document::redo()
{
    if (!m_edits.empty() && (m_undo_pointer >= 0) && (m_undo_pointer < static_cast<int>(m_edits.size()))) {
        m_edits[m_undo_pointer++].undo(*this);
    }
}

void Document::up(bool select)
{
    int line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    if (line > 0) {
        move_point(clamp(m_lines[line - 1].start_index + column, m_lines[line - 1].start_index, m_lines[line - 1].start_index + line_length(line - 1)));
    }
    update_internals(select, line - 1);
}

void Document::down(bool select)
{
    int line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    if (line < (line_count() - 1)) {
        move_point(clamp(m_lines[line + 1].start_index + column, m_lines[line + 1].start_index, m_lines[line + 1].start_index + line_length(line + 1)));
    }
    update_internals(select, line + 1);
}

void Document::left(bool select)
{
    if (m_point > 0)
        move_point(m_point - 1);
    update_internals(select);
}

void Document::word_left(bool select)
{
    auto point = m_point;
    while (0 < point && !isalnum(m_text[point])) {
        --point;
    }
    while (0 < point && isalnum(m_text[point])) {
        --point;
    }
    move_point(point);
    update_internals(select);
}

void Document::right(bool select)
{
    if (m_point < text_length() - 1)
        move_point(m_point + 1);
    update_internals(select);
}

void Document::word_right(bool select)
{
    auto point = m_point;
    while (point < text_length() - 1 && !isalnum(m_text[point])) {
        ++point;
    }
    while (point < text_length() - 1 && isalnum(m_text[point])) {
        ++point;
    }
    move_point(point);
    update_internals(select);
}

void Document::page_up(bool select)
{
    auto line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    line = clamp(line - rows(), 0, line);
    column = clamp(column, 0, line_length(line));
    move_point(m_lines[line].start_index + column);
    update_internals(select, line);
}

void Document::page_down(bool select)
{
    auto line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    line = clamp(line + rows(), line, line_count() - 1);
    column = clamp(column, 0, line_length(line));
    move_point(m_lines[line].start_index + column);
    update_internals(select, line);
}

void Document::home(bool select)
{
    auto point = m_point;
    for (; point > 1 && m_text[point - 1] != '\n'; --point)
        ;
    move_point(point);
    update_internals(select);
}

void Document::end(bool select)
{
    auto point { m_point };
    for (; point < text_length() && m_text[point] != '\n'; ++point)
        ;
    move_point(point);
    update_internals(select);
}

void Document::top(bool select)
{
    move_to(0, 0, select);
}

void Document::bottom(bool select)
{
    move_to(line_count() - 1, line_length(line_count() - 1), select);
}

bool Document::find(std::string const& term)
{
    m_found = true;
    m_find_term = term;
    return find_next();
}

bool Document::find_next()
{
    if (m_find_term.empty())
        return true;
    auto stash_point = m_point;
    auto stash_mark = m_mark;
    if (!m_found) {
        m_mark = m_point = 0;
    }
    auto where = m_text.find(m_find_term, m_point);
    if (where != std::string::npos) {
        m_mark = static_cast<int>(where);
        move_point(m_mark + static_cast<int>(m_find_term.length()));
        m_found = true;
        update_internals(true);
        return true;
    }
    m_point = stash_point;
    m_mark = stash_mark;
    update_internals(true);
    m_found = false;
    return false;
}

void Document::clear()
{
    add_edit_action(EditAction::move_cursor(m_point, 0));
    add_edit_action(EditAction::delete_text(0, m_text));
    update_internals(false);
}

std::string Document::load(std::string const& file_name)
{
    m_path = fs::absolute(file_name);
    m_filetype = get_filetype(m_path);
    m_parser = std::unique_ptr<ScratchParser>(m_filetype.parser_builder());
    if (auto error_maybe = m_parser->read_file(m_path.string()); error_maybe.is_error())
        return error_maybe.error().message();
    m_point = m_mark = 0;
    m_dirty = false;
    m_changed = true;
    return "";
}

std::string Document::save()
{
    if (!m_dirty)
        return "";
    std::fstream s(m_path.string(), std::fstream::out);
    if (!s.is_open())
        return format("Error opening '{}'", m_path.string());
    s << m_text;
    if (s.fail() || s.bad())
        return format("Error saving '{}'", m_path.string());
    m_dirty = false;
    return "";
}

std::string Document::save_as(std::string const& new_file_name)
{
    m_path = new_file_name;
    m_filetype = get_filetype(m_path);
    m_parser = std::unique_ptr<ScratchParser>(m_filetype.parser_builder());
    return save();
}

void Document::render()
{
    if (!parsed()) {
        m_text.clear();
        m_lines.clear();
        m_lines.emplace_back();
        m_lines.back().start_index = 0;
        auto start = std::chrono::steady_clock::now();
        bool done { false };
        while (!done) {
            auto& token = lex();
            if (token.code() == TokenCode::EndOfFile)
                break;
            switch (token.code()) {
            case TokenCode::NewLine:
                m_text += "\n";
                m_lines.emplace_back();
                m_lines.back().start_index = text_length();
                break;
            case TokenCode::EndOfFile:
                done = true;
                break;
            default:
                m_lines.back().tokens.push_back(token);
                m_text += token.string_value();
                break;
            }
        }
        m_changed = false;
        m_last_parse_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    }

    auto point_line = find_line_number(m_point);
    auto point_column = m_point - m_lines[point_line].start_index;
    editor()->mark_current_line(point_line - m_screen_top);

    bool has_selection = m_point != m_mark;
    int start_selection = std::min(m_point, m_mark);
    int end_selection = std::max(m_point, m_mark);
    for (auto ix = m_screen_top; ix < line_count() && ix < m_screen_top + editor()->rows(); ++ix) {
        auto const& line = m_lines[ix];
        auto line_len = line_length(ix);
        auto line_end = line.start_index + line_len;
        if (has_selection && (start_selection <= line_end) && (end_selection >= line.start_index)) {
            int start_block = start_selection - line.start_index;
            if (start_block < 0)
                start_block = 0;
            int end_block = end_selection - line.start_index;
            if (end_block > line_len)
                end_block = editor()->columns();
            int block_width = end_block - start_block;
            if (block_width > 0) {
                SDL_Rect r {
                    start_block * App::instance().context()->character_width(),
                    editor()->line_top(ix - m_screen_top),
                    block_width * App::instance().context()->character_width(),
                    editor()->line_height()
                };
                editor()->box(r, App::instance().color(PaletteIndex::Selection));
            }
        }

        auto len = 0u;
        for (auto const& token : line.tokens) {
            auto t = token.value();
            if (len + t.length() < static_cast<size_t>(m_screen_left)) {
                len += t.length();
                continue;
            } else if (len < static_cast<size_t>(m_screen_left)) {
                t = t.substr(m_screen_left - len);
            } else if (len + t.length() > static_cast<size_t>(m_screen_left + columns())) {
                t = t.substr(0, m_screen_left + columns() - len);
            }
            editor()->append(m_parser->colorize(token.code(), t));
            len += token.value().length();
            if (len >= static_cast<size_t>(m_screen_left + editor()->columns()))
                break;
        }
        editor()->newline();
    }

    editor()->text_cursor(point_line - m_screen_top, point_column - m_screen_left);
}

bool Document::dispatch(SDL_Keysym sym)
{
    switch (sym.sym) {
    case SDLK_ESCAPE:
        m_mark = m_point;
        break;
    case SDLK_UP:
        up(sym.mod & KMOD_SHIFT);
        break;
    case SDLK_PAGEUP:
        page_up(sym.mod & KMOD_SHIFT);
        break;
    case SDLK_DOWN:
        down(sym.mod & KMOD_SHIFT);
        break;
    case SDLK_PAGEDOWN:
        page_down(sym.mod & KMOD_SHIFT);
        break;
    case SDLK_LEFT:
        if (sym.mod & KMOD_GUI)
            word_left(sym.mod & KMOD_SHIFT);
        else
            left(sym.mod & KMOD_SHIFT);
        break;
    case SDLK_RIGHT:
        if (sym.mod & KMOD_GUI)
            word_right(sym.mod & KMOD_SHIFT);
        else
            right(sym.mod & KMOD_SHIFT);
        break;
    case SDLK_HOME:
        if (sym.mod & KMOD_GUI) {
            move_to(0, 0, sym.mod & KMOD_SHIFT);
        } else {
            home(sym.mod & KMOD_SHIFT);
        }
        break;
    case SDLK_END:
        if (sym.mod & KMOD_GUI) {
            move_to(line_count() - 1, line_length(line_count() - 1), sym.mod & KMOD_SHIFT);
        } else {
            end(sym.mod & KMOD_SHIFT);
        }
        break;
    case SDLK_BACKSPACE:
    case SDLK_DELETE:
        if (m_point == m_mark)
            extend_selection((sym.sym == SDLK_BACKSPACE) ? -1 : 1);
        erase_selection();
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        split_line();
        break;
    default:
        return false;
    }
    return true;
}

void Document::mousedown(int line, int column)
{
    move_to(m_screen_top + line, m_screen_left + column, false);
}

void Document::motion(int line, int column)
{
    move_to(m_screen_top + line, m_screen_left + column, true);
}

void Document::click(int line, int column, int clicks)
{
    switch (clicks) {
    case 2:
        select_word();
        break;
    case 3:
        select_line();
        break;
    default:
        break;
    }
}

void Document::wheel(int lines)
{
    m_screen_top = clamp(m_screen_top + lines, 0, line_count() - 1);
}

void Document::handle_text_input()
{
    insert(App::instance().input_buffer());
}

Token const& Document::lex()
{
    return m_parser->next_token();
}

void Document::rewind()
{
    m_parser->rewind();
}

std::optional<ScheduledCommand> Document::command(std::string const& name) const
{
    if (auto ret = Widget::command(name); ret.has_value())
        return ret;
    if (m_parser != nullptr) {
        if (auto ret = m_parser->command(name); ret.has_value())
            return ret;
    }
    return {};
}

std::vector<ScheduledCommand> Document::commands() const
{
    auto ret = Widget::commands();
    if (m_parser != nullptr) {
        for (auto cmd : m_parser->commands())
            ret.push_back({ const_cast<Document&>(*this), cmd });
    }
    return ret;
}

}
