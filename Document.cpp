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
        for (auto const &e : type.extensions) {
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

std::string const& Document::line(size_t line_no) const
{
    assert(line_no < m_lines.size());
    return m_lines[line_no].text;
}

void Document::assign_to_parser()
{
    std::string s;
    for (auto const& l : m_lines) {
        s += l.text + "\n";
    }
    while (s.length() > 2 && s.substr(s.length() - 2) == "\n\n")
        s = s.substr(0, s.length() - 1);
    m_parser->assign(s);
    invalidate();
}

size_t Document::line_length(size_t line_no) const
{
    assert(line_no < m_lines.size());
    return m_lines[line_no].text.length();
}

size_t Document::line_count() const
{
    return m_lines.size();
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
    if (m_point.line >= m_lines.size()) {
        m_lines.emplace_back();
    } else {
        auto right = m_lines[m_point.line].text.substr(m_point.column);
        m_lines[m_point.line].text.erase(m_point.column);
        auto iter = m_lines.begin();
        for (auto ix = 0u; ix <= m_point.line; ++ix, ++iter)
            ;
        m_lines.insert(iter, Line { right });
    }
    m_point.line++;
    m_point.column = 0;
    assign_to_parser();
}

void Document::join_lines()
{
    if (m_point.line > 0 && m_point.line < m_lines.size()) {
        m_lines[m_point.line - 1].text += m_lines[m_point.line].text;
        auto iter = m_lines.begin() + m_point.line;
        m_lines.erase(iter);
        m_point.line--;
        m_point.column = line_length(m_point.line);
        assign_to_parser();
    }
}

void Document::insert(std::string const& str)
{
    if (str.empty())
        return;
    erase_selection();
    if (m_point.line < m_lines.size()) {
        if (m_point.column <= m_lines[m_point.line].text.length()) {
            m_lines[m_point.line].text.insert(m_point.column, str);
            m_point.column += str.length();
            assign_to_parser();
        }
    } else if (m_point.line == m_lines.size() && (m_point.column == 0)) {
        m_lines.emplace_back(str);
        m_point.column = str.length();
        assign_to_parser();
    }
}

void Document::reset_selection()
{
    m_mark = m_point;
}

void Document::extend_selection(int num)
{
    DocumentPosition& left = (m_mark <= m_point) ? m_mark : m_point;
    DocumentPosition& right = (m_mark <= m_point) ? m_point : m_mark;

    if (num < 0) {
        num = -num;
        for (; num > 0; --num) {
            if (left.column > 0) {
                --left.column;
            } else if (left.line == 0) {
                return;
            } else {
                --left.line;
                left.column = line_length(left.line);
            }
        }
        return;
    }

    for (;num > 0; --num) {
        if (right.column < line_length(right.line)) {
            ++left.column;
        } else if (left.line == line_count() - 1) {
            return;
        } else {
            ++left.line;
            left.column = 0;
        }
    }
}

std::string Document::selected_text()
{
    if (m_point == m_mark)
        return "";
    DocumentPosition start_selection = std::min(m_point, m_mark);
    DocumentPosition end_selection = std::max(m_point, m_mark);
    std::string ret;
    for (auto ix = start_selection.line; ix <= end_selection.line; ++ix) {
        if (ix > start_selection.line)
            ret += '\n';
        auto& line = m_lines[ix];
        int start_block = 0;
        if (ix == start_selection.line)
            start_block = start_selection.column;
        int end_block = line.text.length();
        if (ix == end_selection.line)
            end_block = end_selection.column - start_block;
        if (start_block == 0 && end_block == line.text.length()) {
            ret += line.text;
        } else {
            ret += line.text.substr(start_block, end_block-start_block);
        }
    }
    return ret;
}

void Document::erase_selection()
{
    if (m_point == m_mark)
        return;
    DocumentPosition start_selection = std::min(m_point, m_mark);
    DocumentPosition end_selection = std::max(m_point, m_mark);

    if (start_selection.line == end_selection.line) {
        m_lines[start_selection.line].text.erase(start_selection.column, end_selection.column - start_selection.column);
        m_point = start_selection;
        assign_to_parser();
        return;
    }

    m_lines[start_selection.line].text.erase(start_selection.column);
    m_lines[start_selection.line].text += m_lines[end_selection.line].text.substr(end_selection.column);
    m_lines.erase(m_lines.begin() + start_selection.line + 1, m_lines.begin() + end_selection.line + 1);
    m_point = start_selection;
    assign_to_parser();
}

void Document::copy_to_clipboard()
{
    auto selection = selected_text();
    if (selection.empty())
        return;
    if (auto err = SDL_SetClipboardText(selection.c_str()); err != 0) {
        fatal("Error copying selection to clipboard: {}", SDL_GetError());
    }
}

void Document::cut_to_clipboard()
{
    if (m_point == m_mark)
        return;
    copy_to_clipboard();
    erase_selection();
}

void Document::paste_from_clipboard()
{
    if (SDL_HasClipboardText() == SDL_FALSE)
        return;
    auto clipboard = SDL_GetClipboardText();
    if ((clipboard == nullptr) || !clipboard[0])
        fatal("Error retrieving clipboard text: {}", SDL_GetError());
    insert(clipboard);
}

void Document::move_to(int line, int column)
{
    if ((line >= 0) && (line < m_lines.size())) {
        m_point.line = line;
        if (m_point.column >= line_length(line))
            m_point.column = line_length(line);
    }
    if ((column >= 0) && (column < line_length(m_point.line)))
        m_point.column = column;
    m_mark = m_point;
}

void Document::up(Editor*, bool select)
{
    if (m_point.line > 0) {
        m_point.line--;
        if (m_point.column > line_length(m_point.line))
            m_point.column = line_length(m_point.line);
        if (m_screen_top > m_point.line - 1)
            --m_screen_top;
        debug(scratch, "KEY_UP: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
    }
    if (!select)
        m_mark = m_point;
}

void Document::down(Editor* editor, bool select)
{
    if (m_point.line < (line_count() - 1)) {
        m_point.line++;
        if (m_point.column > line_length(m_point.line))
            m_point.column = line_length(m_point.line);
        if (m_point.line - m_screen_top >= editor->rows())
            ++m_screen_top;
        debug(scratch, "KEY_DOWN: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
    }
    if (!select)
        m_mark = m_point;
}

void Document::left(Editor*, bool select)
{
    if (m_point.column > 0) {
        --m_point.column;
        if (m_screen_left > m_point.column - 1)
            --m_screen_left;
    } else if (m_point.line > 0) {
        --m_point.line;
        m_point.column = line_length(m_point.line);
        m_screen_left = 0;
        if (m_screen_top > m_point.line - 1)
            --m_screen_top;
    }
    debug(scratch, "KEY_LEFT: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
    if (!select)
        m_mark = m_point;
}

void Document::word_left(Editor* editor, bool select)
{
    do {
        auto line = m_lines[m_point.line].text;
        while (0 < m_point.column && !isalnum(line[m_point.column])) {
            --m_point.column;
        }
        while (0 < m_point.column && isalnum(line[m_point.column])) {
            --m_point.column;
        }
        if (m_point.column < 0) {
            if (m_point.line == 0)
                m_point.column = 0;
            else
                --m_point.line;
        }
    } while (m_point.column < 0);
    if (!select)
        m_mark = m_point;
}

void Document::right(Editor* editor, bool select)
{
    if (m_point.column < line_length(m_point.line)) {
        ++m_point.column;
        if (m_point.column - m_screen_left > editor->columns())
            ++m_screen_left;
    } else if (m_point.line < line_count()) {
        ++m_point.line;
        m_point.column = 0;
        if (m_point.line - m_screen_top > editor->rows())
            ++m_screen_top;
    }
    debug(scratch, "KEY_RIGHT: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
    if (!select)
        m_mark = m_point;
}

void Document::word_right(Editor* editor, bool select)
{
    auto line = m_lines[m_point.line].text;
    do {
        while (m_point.column < line_length(m_point.line) - 1 && !isalnum(line[m_point.column])) {
            ++m_point.column;
        }
        while (m_point.column < line_length(m_point.line) - 1 && isalnum(line[m_point.column])) {
            ++m_point.column;
        }
        if (m_point.column >= line_length(m_point.line)) {
            if (m_point.line == line_count() - 1)
                m_point.column = line_length(m_point.line) - 1;
            else
                ++m_point.line;
        }
    } while (m_point.column < 0);
    if (!select)
        m_mark = m_point;
}

void Document::page_up(Editor* editor, bool select)
{
    if (m_point.line > 0) {
        m_point.line = clamp((int)m_point.line - editor->rows(), 0, (int)m_point.line - editor->rows());
        m_screen_top = clamp((int)m_screen_top - editor->rows(), 0, (int)m_point.line);
        if (m_point.column > line_length(m_point.line))
            m_point.column = line_length(m_point.line);
        debug(scratch, "KEY_PGUP: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
    }
    if (!select)
        m_mark = m_point;
}

void Document::page_down(Editor* editor, bool select)
{
    if (m_point.line < (line_count() - 1)) {
        m_point.line = clamp((int)m_point.line + editor->rows(), (int)line_count()-editor->rows(), (int)line_count());
        m_screen_top = clamp((int)m_screen_top + editor->rows(), (int)line_count() - editor->rows(), (int)line_count());
        if (m_point.column > line_length(m_point.line))
            m_point.column = line_length(m_point.line);
        debug(scratch, "KEY_PGDN: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
    }
    if (!select)
        m_mark = m_point;
}



void Document::clear()
{
    if (empty())
        return;
    m_lines.clear();
    m_parser->assign("\n");
    m_point.clear();
    m_mark.clear();
    invalidate();
}

std::string Document::load(std::string const& file_name)
{
    m_path = fs::absolute(file_name);
    m_filetype = get_filetype(m_path);
    m_parser = std::unique_ptr<ScratchParser>(m_filetype.parser_builder());
    if (auto error_maybe = m_parser->read_file(m_path.string()); error_maybe.is_error())
        return error_maybe.error().message();
    m_point.clear();
    m_mark.clear();
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
    for (auto const& l : m_lines) {
        s << l.text << "\n";
    }
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

void Document::render(Editor* editor)
{
    if (!parsed()) {
        m_lines.clear();
        m_lines.emplace_back();
        auto count = 0u;
        auto start = std::chrono::steady_clock::now();
        for (auto token = lex(); token.code() != TokenCode::EndOfFile; token = lex()) {
            count++;
            switch (token.code()) {
            case TokenCode::NewLine:
                m_lines.emplace_back();
                break;
            default:
                m_lines.back().tokens.push_back(token);
                m_lines.back().text += token.value();
                break;
            }
        }
        m_last_parse_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    }

    editor->mark_current_line(m_point.line - m_screen_top);
    bool has_selection = m_point != m_mark;
    DocumentPosition start_selection = std::min(m_point, m_mark);
    DocumentPosition end_selection = std::max(m_point, m_mark);
    for (auto ix = m_screen_top; ix < m_lines.size() && ix < m_screen_top + m_editor->rows(); ++ix) {
        auto const& line = m_lines[ix];
        if (has_selection && (ix >= start_selection.line) && (ix <= end_selection.line)) {
            int start_block = 0;
            if (ix == start_selection.line)
                start_block = start_selection.column;
            int block_width = editor->columns() - start_block;
            if (ix == end_selection.line)
                block_width = end_selection.column - start_block;
            if (block_width > 0) {
                SDL_Rect r {
                    start_block * App::instance().context()->character_width(),
                    editor->line_top(ix - m_screen_top),
                    block_width * App::instance().context()->character_width(),
                    editor->line_height()
                };
                editor->box(r, App::instance().color(PaletteIndex::Selection));
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
            } else if (len + t.length() > m_screen_left + m_editor->columns()) {
                t = t.substr(0, m_screen_left + m_editor->columns() - len);
            }
            switch (token.code()) {
            case TokenCode::Comment:
                editor->append(DisplayToken { t, PaletteIndex::Comment });
                break;
            case TokenCode::Identifier:
                editor->append(DisplayToken { t, PaletteIndex::Identifier });
                break;
            case TokenCode::DoubleQuotedString:
                editor->append(DisplayToken { t, PaletteIndex::CharLiteral });
                break;
            case TokenCode::SingleQuotedString:
                editor->append(DisplayToken { t, PaletteIndex::String });
                break;
            case TokenKeyword:
                editor->append(DisplayToken { t, PaletteIndex::Keyword });
                break;
            case TokenConstant:
                editor->append(DisplayToken { t, PaletteIndex::Keyword });
                break;
            case TokenDirective:
                editor->append(DisplayToken { t, PaletteIndex::Preprocessor });
                break;
            default:
                editor->append(DisplayToken { t, PaletteIndex::Punctuation });
                break;
            }
            len += token.value().length();
            if (len >= m_screen_left + m_editor->columns())
                break;
        }
        editor->newline();
    }

    editor->text_cursor(m_point.line - m_screen_top, m_point.column - m_screen_left);
}

bool Document::dispatch(Editor* editor, SDL_Keysym sym)
{
    switch (sym.sym) {
    case SDLK_ESCAPE:
        m_mark = m_point;
        break;
    case SDLK_UP:
        up(editor, sym.mod & KMOD_SHIFT);
        break;
    case SDLK_PAGEUP:
        page_up(editor, sym.mod & KMOD_SHIFT);
        break;
    case SDLK_DOWN:
        down(editor, sym.mod & KMOD_SHIFT);
        break;
    case SDLK_PAGEDOWN:
        page_down(editor, sym.mod & KMOD_SHIFT);
        break;
    case SDLK_LEFT:
        if (sym.mod & KMOD_GUI)
            word_left(editor, sym.mod & KMOD_SHIFT);
        else
            left(editor, sym.mod & KMOD_SHIFT);
        break;
    case SDLK_RIGHT:
        if (sym.mod & KMOD_GUI)
            word_right(editor, sym.mod & KMOD_SHIFT);
        else
            right(editor, sym.mod & KMOD_SHIFT);
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

void Document::handle_text_input()
{
    insert(App::instance().input_buffer());
}

Token Document::lex()
{
    Token token = m_parser->next_token();
    return token;
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
    m_cleared = true;
    m_dirty = true;
    m_mark = m_point;
}

}
