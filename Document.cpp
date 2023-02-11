/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>
#include <fstream>

#import <core/Format.h>

#include <App.h>
#include <Document.h>
#include <Editor.h>
#include <Parser/CPlusPlus.h>

using namespace Obelix;
using namespace Scratch::Parser;

namespace Scratch {

FileType s_filetypes[] = {
    // Plain Text parser must be in slot 0! Do not sort down!
    { { ".txt" }, "text/plain", []() -> ScratchParser* {
         return new PlainTextParser();
     } },
    { { ".cpp", ".h", ".hpp" }, "text/x-cpp", []() -> ScratchParser* {
         return new CPlusPlusParser();
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

Document::Document(Editor* editor)
    : m_editor(editor)
{
    m_path.clear();
    m_lines.emplace_back();
    m_filetype = get_filetype("");
    m_parser = std::unique_ptr<ScratchParser>(m_filetype.parser_builder());
}

std::string Document::line(size_t line_no) const
{
    assert(line_no < m_lines.size());
    if (line_no == m_lines.size() - 1)
        return m_text.substr(m_lines[line_no].start_index);
    return m_text.substr(m_lines[line_no].start_index, m_lines[line_no + 1].start_index);
}

void Document::assign_to_parser()
{
    m_parser->assign(m_text);
    invalidate();
}

int Document::line_length(size_t line_no) const
{
    assert(line_no < m_lines.size());
    if (line_no == m_lines.size() - 1)
        return static_cast<int>(m_text.length()) - m_lines[line_no].start_index;
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
    if (m_point >= m_text.length()) {
        m_text += "\n";
    } else {
        m_text.insert(m_point, "\n");
    }
    m_point++;
    assign_to_parser();
}

void Document::join_lines()
{
    int ix;
    for (ix = m_point; (ix > 0) && (m_text[ix] != '\n'); --ix)
        ;
    if (ix > 0) {
        m_text.erase(ix, 1);
        m_point = ix - 1;
        assign_to_parser();
    }
}

void Document::insert(std::string const& str)
{
    if (str.empty())
        return;
    erase_selection();
    m_text.insert(m_point, str);
    m_point += static_cast<int>(str.length());
    assign_to_parser();
}

void Document::reset_selection()
{
    m_mark = m_point;
}

void Document::extend_selection(int num)
{
    int& left = (m_mark <= m_point) ? m_mark : m_point;
    int& right = (m_mark <= m_point) ? m_point : m_mark;

    if (num < 0) {
        left += num;
        if (left < 0)
            left = 0;
    } else {
        right += num;
        if (right > m_text.length())
            right = static_cast<int>(m_text.length());
    }
}

std::string Document::selected_text()
{
    if (m_point == m_mark)
        return "";
    int start_selection = std::min(m_point, m_mark);
    int end_selection = std::max(m_point, m_mark);
    return m_text.substr(start_selection, end_selection - start_selection);
}

void Document::erase_selection()
{
    if (m_point == m_mark)
        return;
    int start_selection = std::min(m_point, m_mark);
    int end_selection = std::max(m_point, m_mark);
    m_text.erase(start_selection, end_selection - start_selection);
    m_point = start_selection;
    m_mark = m_point;
    assign_to_parser();
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
    int line_max = static_cast<int>(m_lines.size()) - 1;
    while (true) {
        int line = line_min + (line_max - line_min) / 2;
        if ((line < m_lines.size() - 1 && m_lines[line].start_index <= cursor && cursor < m_lines[line + 1].start_index) || (line == m_lines.size() - 1 && m_lines[line].start_index <= cursor)) {
            return line;
        } else {
            if (m_lines[line].start_index > cursor) {
                line_max = line;
            } else {
                line_min = line;
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

void Document::move_to(int line, int column, bool select)
{
    line = clamp(line, 0, (int)line_count() - 1);
    column = clamp(column, 0, (int)line_length(line));
    m_point = m_lines[line].start_index + column;
    if (m_screen_top > line || m_screen_top + rows() < line)
        m_screen_top = line - rows() / 2;
    if (m_screen_left > column || m_screen_left + columns() < column)
        m_screen_left = column - columns() / 2;
    update_internals_after_move(select);
}

void Document::update_internals_after_move(bool select, int line)
{
    if (line < 0)
        line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    m_screen_top = clamp(m_screen_top, std::max(0, line - m_editor->rows() + 1), line);
    m_screen_left = clamp(m_screen_left, std::max(0, column - m_editor->columns() + 1), column);
    if (!select)
        m_mark = m_point;
}

void Document::up(bool select)
{
    int line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    if (line > 0) {
        m_point = clamp(m_lines[line - 1].start_index + column, m_lines[line - 1].start_index, m_lines[line - 1].start_index + line_length(line - 1));
    }
    update_internals_after_move(select, line - 1);
}

void Document::down(bool select)
{
    int line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    if (line < (line_count() - 1)) {
        m_point = clamp(m_lines[line + 1].start_index + column, m_lines[line + 1].start_index, m_lines[line + 1].start_index + line_length(line + 1));
    }
    update_internals_after_move(select, line + 1);
}

void Document::left(bool select)
{
    if (m_point > 0)
        --m_point;
    update_internals_after_move(select);
}

void Document::word_left(bool select)
{
    while (0 < m_point && !isalnum(m_text[m_point])) {
        --m_point;
    }
    while (0 < m_point && isalnum(m_text[m_point])) {
        --m_point;
    }
    update_internals_after_move(select);
}

void Document::right(bool select)
{
    if (m_point < m_text.length() - 1)
        ++m_point;
    update_internals_after_move(select);
}

void Document::word_right(bool select)
{
    while (m_point < m_text.length() - 1 && !isalnum(m_text[m_point])) {
        ++m_point;
    }
    while (m_point < m_text.length() - 1 && isalnum(m_text[m_point])) {
        ++m_point;
    }
    update_internals_after_move(select);
}

void Document::page_up(bool select)
{
    auto line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    line = clamp(line - rows(), 0, line);
    column = clamp(column, 0, line_length(line));
    m_point = m_lines[line].start_index + column;
    update_internals_after_move(select, line);
}

void Document::page_down(bool select)
{
    auto line = find_line_number(m_point);
    int column = m_point - m_lines[line].start_index;
    line = clamp(line + rows(), line, static_cast<int>(m_lines.size()));
    column = clamp(column, 0, line_length(line));
    m_point = m_lines[line].start_index + column;
    update_internals_after_move(select, line);
}

void Document::home(bool select)
{
    for (; m_point > 1 && m_text[m_point - 1] != '\n'; --m_point)
        ;
    update_internals_after_move(select);
}

void Document::end(bool select)
{
    for (; m_point < m_text.length() && m_text[m_point] != '\n'; ++m_point)
        ;
    update_internals_after_move(select);
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
    if (where != m_text.npos) {
        m_mark = where;
        m_point = m_mark + m_find_term.length();
        m_found = true;
        return true;
    }
    m_point = stash_point;
    m_mark = stash_mark;
    m_found = false;
    return false;
}

void Document::clear()
{
    if (empty())
        return;
    m_lines.clear();
    m_text.clear();
    m_parser->assign("\n");
    m_point = 0;
    m_mark = 0;
    m_point = m_screen_left = m_screen_top = 0;
    invalidate();
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

fs::path const& Document::path() const
{
    return m_path;
}

int Document::rows() const
{
    return m_editor->rows();
}

int Document::columns() const
{
    return m_editor->columns();
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
                m_lines.back().start_index = static_cast<int>(m_text.length());
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
        m_last_parse_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    }

    auto point_line = find_line_number(m_point);
    auto point_column = m_point - m_lines[point_line].start_index;
    m_editor->mark_current_line(point_line - m_screen_top);

    bool has_selection = m_point != m_mark;
    int start_selection = std::min(m_point, m_mark);
    int end_selection = std::max(m_point, m_mark);
    for (auto ix = m_screen_top; ix < m_lines.size() && ix < m_screen_top + m_editor->rows(); ++ix) {
        auto const& line = m_lines[ix];
        auto line_len = line_length(ix);
        auto line_end = line.start_index + line_len;
        if (has_selection && (start_selection <= line_end) && (end_selection >= line.start_index)) {
            int start_block = start_selection - line.start_index;
            if (start_block < 0)
                start_block = 0;
            int end_block = end_selection - line.start_index;
            if (end_block > line_len)
                end_block = m_editor->columns();
            int block_width = end_block - start_block;
            if (block_width > 0) {
                SDL_Rect r {
                    start_block * App::instance().context()->character_width(),
                    m_editor->line_top(ix - m_screen_top),
                    block_width * App::instance().context()->character_width(),
                    m_editor->line_height()
                };
                m_editor->box(r, App::instance().color(PaletteIndex::Selection));
            }
        }

        auto len = 0u;
        for (auto const& token : line.tokens) {
            auto t = token.value();
            if (len + t.length() < m_screen_left) {
                len += t.length();
                continue;
            } else if (len < m_screen_left) {
                t = t.substr(m_screen_left - len);
            } else if (len + t.length() > m_screen_left + columns()) {
                t = t.substr(0, m_screen_left + columns() - len);
            }
            switch (token.code()) {
            case TokenCode::Comment:
                m_editor->append(DisplayToken { t, PaletteIndex::Comment });
                break;
            case TokenCode::Identifier:
                m_editor->append(DisplayToken { t, PaletteIndex::Identifier });
                break;
            case TokenCode::DoubleQuotedString:
                m_editor->append(DisplayToken { t, PaletteIndex::CharLiteral });
                break;
            case TokenCode::SingleQuotedString:
                m_editor->append(DisplayToken { t, PaletteIndex::String });
                break;
            case TokenKeyword:
            case TokenConstant:
                m_editor->append(DisplayToken { t, PaletteIndex::Keyword });
                break;
            case TokenDirective:
                m_editor->append(DisplayToken { t, PaletteIndex::Preprocessor });
                break;
            default:
                m_editor->append(DisplayToken { t, PaletteIndex::Punctuation });
                break;
            }
            len += token.value().length();
            if (len >= m_screen_left + m_editor->columns())
                break;
        }
        m_editor->newline();
    }

    m_editor->text_cursor(point_line - m_screen_top, point_column - m_screen_left);
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
            move_to(line_count()-1, line_length(line_count()-1), sym.mod & KMOD_SHIFT);
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

void Document::handle_click(int line, int column)
{
    move_to(m_screen_top + line, m_screen_left + column, false);
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

void Document::invalidate()
{
    m_lines.clear();
    m_lines.emplace_back();
    m_parser->invalidate();
    m_dirty = true;
    m_mark = m_point;
}

}
