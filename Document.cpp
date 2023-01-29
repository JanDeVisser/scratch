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

using namespace Obelix;

namespace Scratch {

Document::Document(Editor* editor)
    : m_editor(editor)
{
    m_path.clear();
    m_lines.emplace_back();
    m_parser.lexer().add_scanner<Obelix::QStringScanner>("\"'", true);
    m_parser.lexer().add_scanner<Obelix::IdentifierScanner>();
    m_parser.lexer().add_scanner<Obelix::NumberScanner>(Obelix::NumberScanner::Config { true, false, true, false, true });
    m_parser.lexer().add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    m_parser.lexer().add_scanner<Obelix::CommentScanner>(true,
        Obelix::CommentScanner::CommentMarker { false, false, "/*", "*/" },
        Obelix::CommentScanner::CommentMarker { false, true, "//", "" });
    m_parser.lexer().add_scanner<Obelix::KeywordScanner>(
        Token(KeywordAuto, "auto"),
        Token(KeywordBreak, "break"),
        Token(KeywordCase, "case"),
        Token(KeywordClass, "class"),
        Token(KeywordConst, "const"),
        Token(KeywordContinue, "continue"),
        Token(KeywordDefault, "default"),
        Token(KeywordElse, "else"),
        Token(KeywordEnum, "enum"),
        Token(KeywordFor, "for"),
        Token(KeywordIf, "if"),
        Token(KeywordNamespace, "namespace"),
        Token(KeywordReturn, "return"),
        Token(KeywordStatic, "static"),
        Token(KeywordStruct, "struct"),
        Token(KeywordSwitch, "switch"),
        Token(KeywordUsing, "using"),
        Token(KeywordWhile, "while"),

        Token(KeywordTrue, "true"),
        Token(KeywordFalse, "false"),
        Token(KeywordNullptr, "nullptr"),

        Token(KeywordDefine, "#define"),
        Token(KeywordElif, "#elif"),
        Token(KeywordElifdef, "#elifdef"),
        Token(KeywordHashElse, "#else"),
        Token(KeywordEndif, "#endif"),
        Token(KeywordHashIf, "#if"),
        Token(KeywordIfdef, "#ifdef"),
        Token(KeywordInclude, "#include"),
        Token(KeywordPragma, "#pragma"));
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
    m_parser.assign(s);
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
    return !m_parser.tokens().empty();
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

void Document::clear()
{
    if (empty())
        return;
    m_lines.clear();
    m_parser.assign("\n");
    m_point.clear();
    m_mark.clear();
    invalidate();
}

std::string Document::load(std::string const& file_name)
{
    m_path = fs::absolute(file_name);
    if (auto error_maybe = m_parser.read_file(m_path.string()); error_maybe.is_error())
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
        auto end = std::chrono::steady_clock::now();
        auto elapsed = (long)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        debug(scratch, "Parsing done. Generated {} tokens in {}ms", count, elapsed);
    }

    auto start_render = std::chrono::steady_clock::now();
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
                    start_block * Editor::char_width,
                    editor->line_top(ix - m_screen_top),
                    block_width * Editor::char_width,
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
                editor->append(DisplayToken { t });
                break;
            }
            len += token.value().length();
            if (len >= m_screen_left + m_editor->columns())
                break;
        }
        editor->newline();
    }

    editor->text_cursor(m_point.line - m_screen_top, m_point.column - m_screen_left);

    auto end_render = std::chrono::steady_clock::now();
    auto elapsed_render = (long)std::chrono::duration_cast<std::chrono::milliseconds>(end_render - start_render).count();
    // debug(scratch, "Rendering done in {}ms", elapsed_render);
}

bool Document::dispatch(Editor* editor, SDL_Keysym sym)
{
    switch (sym.sym) {
    case SDLK_ESCAPE:
        m_mark = m_point;
        break;
    case SDLK_UP:
        if (m_point.line > 0) {
            m_point.line--;
            if (m_point.column > line_length(m_point.line))
                m_point.column = line_length(m_point.line);
            if (m_screen_top > m_point.line - 1)
                --m_screen_top;
            debug(scratch, "KEY_UP: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
        }
        if (!(sym.mod & KMOD_SHIFT))
            m_mark = m_point;
        break;
    case SDLK_PAGEUP:
        if (m_point.line > 0) {
            if (m_point.line < editor->rows()) {
                m_point.line = 0;
                m_screen_top = 0;
            } else {
                m_point.line = clamp((int)m_point.line - editor->rows(), 0, (int)m_point.line - editor->rows());
                m_screen_top = clamp((int)m_screen_top - editor->rows(), 0, (int)m_point.line);
            }
            if (m_point.column > line_length(m_point.line))
                m_point.column = line_length(m_point.line);
            debug(scratch, "KEY_PGUP: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
        }
        if (!(sym.mod & KMOD_SHIFT))
            m_mark = m_point;
        break;
    case SDLK_DOWN:
        if (m_point.line < (line_count() - 1)) {
            m_point.line++;
            if (m_point.column > line_length(m_point.line))
                m_point.column = line_length(m_point.line);
            if (m_point.line - m_screen_top >= editor->rows())
                ++m_screen_top;
            debug(scratch, "KEY_DOWN: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point.line, m_point.column);
        }
        if (!(sym.mod & KMOD_SHIFT))
            m_mark = m_point;
        break;
    case SDLK_LEFT:
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
        if (!(sym.mod & KMOD_SHIFT))
            m_mark = m_point;
        break;
    case SDLK_RIGHT:
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
        if (!(sym.mod & KMOD_SHIFT))
            m_mark = m_point;
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
    Token token;
    if (!m_pending.empty()) {
        token = m_pending.front();
        m_pending.pop_front();
        return token;
    }

    while (m_pending.empty()) {
        token = m_parser.lex();
        switch (token.code()) {
        case TokenCode::NewLine:
            m_pending.emplace_back(token);
            break;

        case KeywordInclude:
            m_pending.emplace_back(TokenDirective, token.value());
            parse_include();
            break;

        case KeywordDefine:
            m_pending.emplace_back(TokenDirective, token.value());
            parse_define();
            break;

        case KeywordHashIf:
        case KeywordElif:
        case KeywordPragma:
            m_pending.emplace_back(TokenDirective, token.value());
            parse_hashif();
            break;

        case KeywordIfdef:
        case KeywordElifdef:
            m_pending.emplace_back(TokenDirective, token.value());
            parse_ifdef();
            break;

        case KeywordEndif:
        case KeywordHashElse:
            m_pending.emplace_back(TokenDirective, token.value());
            break;

        case KeywordAuto:
        case KeywordConst:
        case KeywordIf:
        case KeywordElse:
        case KeywordNamespace:
        case KeywordWhile:
        case KeywordClass:
        case KeywordStruct:
        case KeywordEnum:
        case KeywordFor:
        case KeywordSwitch:
        case KeywordCase:
        case KeywordBreak:
        case KeywordContinue:
        case KeywordDefault:
        case KeywordStatic:
        case KeywordUsing:
            m_pending.emplace_back(TokenKeyword, token.value());
            break;

        case KeywordNullptr:
        case KeywordTrue:
        case KeywordFalse:
            m_pending.emplace_back(TokenConstant, token.value());
            break;

        default:
            m_pending.emplace_back(token);
            break;
        }
    }

    token = m_pending.front();
    m_pending.pop_front();
    return token;
}

Token Document::skip_whitespace()
{
    Token t = m_parser.peek();
    if (t.code() == TokenCode::Whitespace) {
        t = m_parser.lex();
        m_pending.emplace_back(t);
        t = m_parser.peek();
    }
    return t;
}

Token Document::get_next(TokenCode code)
{
    Token t = m_parser.lex();
    m_pending.emplace_back((code != TokenCode::Unknown) ? code : t.code(), t.value());
    return skip_whitespace();
}

void Document::parse_include()
{
    Token t = skip_whitespace();
    switch (t.code()) {
    case Obelix::TokenCode::DoubleQuotedString:
        m_parser.lex();
        m_pending.emplace_back(TokenDirectiveParam, t.value());
        break;
    case TokenCode::LessThan: {
        m_parser.lex();
        auto include = t.value();
        do {
            t = m_parser.lex();
            include += t.value();
        } while (t.code() != Obelix::TokenCode::GreaterThan);
        m_pending.emplace_back(TokenDirectiveParam, include);
        break;
    }
    default:
        break;
    }
}

void Document::parse_define()
{
    Token t = skip_whitespace();
    if (t.code() != TokenCode::Identifier)
        return;
    t = get_next(TokenMacroName);
    if (t.code() == TokenCode::OpenParen) {
        t = get_next();
        while (true) {
            if (t.code() != TokenCode::Identifier)
                return;
            t = get_next(TokenMacroParam);
            if (t.code() == TokenCode::CloseParen) {
                m_parser.lex();
                m_pending.emplace_back(t);
                break;
            }
            if (t.code() != TokenCode::Comma)
                return;
            t = get_next();
        }
    }

    auto escape { false };
    std::string def_string;
    skip_whitespace();
    while (true) {
        t = m_parser.peek();
        switch (t.code()) {
        case TokenCode::Comment:
            m_pending.emplace_back(TokenMacroExpansion, def_string);
            return;
        case TokenCode::Backslash: {
            m_parser.lex();
            escape = !escape;
            def_string += t.value();
            break;
        }
        case TokenCode::NewLine:
            m_parser.lex();
            m_pending.emplace_back(TokenMacroExpansion, def_string);
            if (!escape)
                return;
            def_string = "";
            escape = false;
            m_pending.emplace_back(TokenCode::NewLine, "\n");
            break;
        default:
            escape = false;
            m_parser.lex();
            def_string += t.value();
            break;
        }
    }
}

void Document::parse_ifdef()
{
    Token t = skip_whitespace();
    if (t.code() != TokenCode::Identifier)
        return;
    m_parser.lex();
    m_pending.emplace_back(TokenDirectiveParam, t.value());
}

void Document::parse_hashif()
{
    Token t = skip_whitespace();
    auto escape { false };
    std::string expr;
    while (true) {
        t = m_parser.peek();
        switch (t.code()) {
        case TokenCode::Comment:
            m_pending.emplace_back(TokenDirectiveParam, expr);
            return;
        case TokenCode::Backslash: {
            m_parser.lex();
            escape = !escape;
            expr += t.value();
            break;
        }
        case TokenCode::NewLine:
            m_parser.lex();
            m_pending.emplace_back(TokenDirectiveParam, expr);
            if (!escape)
                return;
            expr = "";
            escape = false;
            m_pending.emplace_back(TokenCode::NewLine, "\n");
            break;
        default:
            escape = false;
            m_parser.lex();
            expr += t.value();
            break;
        }
    }
}

void Document::rewind()
{
    m_parser.rewind();
}

void Document::invalidate()
{
    m_lines.clear();
    m_lines.emplace_back();
    m_parser.invalidate();
    m_cleared = true;
    m_dirty = true;
    m_mark = m_point;
}

}
