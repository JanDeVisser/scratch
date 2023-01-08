/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <unistd.h>

#import <core/Format.h>

#include <App.h>
#include <Document.h>
#include <Editor.h>

using namespace Obelix;

namespace Scratch {

Document::Document(Editor *editor)
    : m_editor(editor)
{
    m_lines.emplace_back();
    m_parser.lexer().add_scanner<Obelix::QStringScanner>("\"'", true);
    m_parser.lexer().add_scanner<Obelix::IdentifierScanner>();
    m_parser.lexer().add_scanner<Obelix::NumberScanner>(Obelix::NumberScanner::Config { true, false, true, false, true });
    m_parser.lexer().add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    m_parser.lexer().add_scanner<Obelix::CommentScanner>(true,
        Obelix::CommentScanner::CommentMarker { false, false, "/*", "*/" },
        Obelix::CommentScanner::CommentMarker { false, true, "//", "" });
    m_parser.lexer().add_scanner<Obelix::KeywordScanner>(
        Token(KeywordIf, "if"),
        Token(KeywordElse, "else"),
        Token(KeywordWhile, "while"),
        Token(KeywordTrue, "true"),
        Token(KeywordFalse, "false"),
        Token(KeywordReturn, "return"),
        Token(KeywordBreak, "break"),
        Token(KeywordContinue, "continue"),
        Token(KeywordSwitch, "switch"),
        Token(KeywordCase, "case"),
        Token(KeywordDefault, "default"),
        Token(KeywordFor, "for"),
        Token(KeywordConst, "const"),
        Token(KeywordStruct, "struct"),
        Token(KeywordClass, "class"),
        Token(KeywordStatic, "static"),
        Token(KeywordEnum, "enum"),
        Token(KeywordNamespace, "namespace"),
        Token(KeywordNullptr, "nullptr"),
        Token(KeywordInclude, "#include"),
        Token(KeywordDefine, "#define"),
        Token(KeywordIfdef, "#ifdef"),
        Token(KeywordHashIf, "#if"),
        Token(KeywordHashElse, "#else"),
        Token(KeywordElif, "#elif"),
        Token(KeywordElifdef, "#elifdef"),
        Token(KeywordEndif, "#endif"),
        Token(KeywordPragma, "#pragma")
        );
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
    while (s.length() > 2 && s.substr(s.length()-2) == "\n\n")
        s = s.substr(0, s.length()-1);
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

size_t Document::parsed() const
{
    return !m_parser.tokens().empty();
}

void Document::backspace(size_t column, size_t row)
{
    assert(row < m_lines.size());
    assert(column <= m_lines[row].text.length());
    assert(column > 0 || row > 0);
    if (column > 0) {
        m_lines[row].text.erase(column - 1, 1);
    } else {
        join_lines(row-1);
    }
    assign_to_parser();
}

void Document::split_line(size_t column, size_t row)
{
    if (row >= m_lines.size()) {
        m_lines.emplace_back();
        invalidate();
        return;
    }
    auto right = m_lines[row].text.substr(column);
    m_lines[row].text.erase(column);
    auto iter = m_lines.begin();
    for (auto ix = 0u; ix <= row; ++ix, ++iter);
    m_lines.insert(iter, { right, {} });
    assign_to_parser();
}

void Document::join_lines(size_t row)
{
    assert(row < m_lines.size()-1);
    m_lines[row].text += m_lines[row+1].text;
    auto iter = m_lines.begin();
    for (auto ix = 0u; ix < row+1; ++ix, ++iter);
    m_lines.erase(iter);
    invalidate();
}

void Document::insert(size_t column, size_t row, char ch)
{
    assert(row < m_lines.size());
    assert(column <= m_lines[row].text.length());
    m_lines[row].text.insert(column, 1, ch);
    assign_to_parser();
}

void Document::clear()
{
    m_lines.clear();
    m_parser.assign("\n");
    invalidate();
}

std::string Document::load(std::string file_name)
{
    m_filename = std::move(file_name);
    if (auto error_maybe = m_parser.read_file(m_filename); error_maybe.is_error())
        return error_maybe.error().message();
    return "";
}

std::string Document::save()
{
    std::fstream s(filename(), std::fstream::out);
    if (!s.is_open())
        return format("Error opening '{}'", filename());
    for (auto const& l : m_lines) {
        s << l.text + "\n";
    }
    if (s.fail() || s.bad())
        return format("Error saving '{}'", filename());
    return "";
}

std::string Document::save_as(std::string new_file_name)
{
    m_filename = std::move(new_file_name);
    return save();
}

std::string const& Document::filename() const
{
    return m_filename;
}

void Document::render(Editor *editor)
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
    for (auto ix = m_screen_top; ix < m_lines.size() && ix < m_screen_top + m_editor->rows() - 2; ++ix) {
        auto const& line = m_lines[ix];
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
                editor->append(DisplayToken{ t, PaletteIndex::Identifier });
                break;
            case TokenCode::DoubleQuotedString:
                editor->append(DisplayToken{ t, PaletteIndex::CharLiteral });
                break;
            case TokenCode::SingleQuotedString:
                editor->append(DisplayToken{ t, PaletteIndex::String });
                break;
            case KeywordConst:
            case KeywordIf:
            case KeywordElse:
            case KeywordNamespace:
            case KeywordNullptr:
            case KeywordWhile:
            case KeywordClass:
            case KeywordStruct:
                editor->append(DisplayToken{ t, PaletteIndex::Keyword });
                break;
            case TokenDirective:
                editor->append(DisplayToken{ t, PaletteIndex::Preprocessor });
                break;
            default:
                editor->append(DisplayToken{ t });
                break;
            }
            len += token.value().length();
            if (len >= m_screen_left + m_editor->columns())
                break;
        }
        editor->newline();
    }
    auto end_render = std::chrono::steady_clock::now();
    auto elapsed_render = (long)std::chrono::duration_cast<std::chrono::milliseconds>(end_render - start_render).count();
    debug(scratch, "Rendering done in {}ms", elapsed_render);
}

#if 0
bool Document::handle(KeyCode key)
{
    switch (key) {
    case KEY_UP:
        if (m_point_line > 0) {
            m_point_line--;
            if (m_point_column > line_length(m_point_line))
                m_point_column = line_length(m_point_line);
            if (m_screen_top > m_point_line - 1)
                --m_screen_top;
            debug(scratch, "KEY_UP: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point_line, m_point_column);
        }
        break;
    case KEY_DOWN:
        if (m_point_line < (line_count() - 1)) {
            m_point_line++;
            if (m_point_column > line_length(m_point_line))
                m_point_column = line_length(m_point_line);
            if (m_point_line - m_screen_top > App::instance().rows() - 4)
                ++m_screen_top;
            debug(scratch, "KEY_DOWN: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point_line, m_point_column);
        }
        break;
    case KEY_LEFT:
        if (m_point_column > 0) {
            --m_point_column;
            if (m_screen_left > m_point_column - 1)
                --m_screen_left;
        } else if (m_point_line > 0) {
            --m_point_line;
            m_point_column = line_length(m_point_line);
            m_screen_left = 0;
            if (m_screen_top > m_point_line - 1)
                --m_screen_top;
        }
        debug(scratch, "KEY_LEFT: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point_line, m_point_column);
        break;
    case KEY_RIGHT:
        if (m_point_column < line_length(m_point_line)) {
            ++m_point_column;
            if (m_point_column - m_screen_left > App::instance().columns() - 2)
                ++m_screen_left;
        } else if (m_point_line < line_count()) {
            ++m_point_line;
            m_point_column = 0;
            if (m_point_line - m_screen_top > App::instance().rows() - 4)
                ++m_screen_top;
        }
        debug(scratch, "KEY_RIGHT: screen: {},{} point: {},{}", m_screen_top, m_screen_left, m_point_line, m_point_column);
        break;
    case KEY_BACKSPACE:
    case 127:
    case '\b':
        backspace(m_point_column, m_point_line);
        if (m_point_column > 0) {
            m_point_column--;
        } else if (m_point_line > 0) {
            m_point_line--;
            m_point_column = line_length(m_point_line);
        }
        break;
    case 13:
        split_line(m_point_column, m_point_line);
        ++m_point_line;
        m_point_column = 0;
        break;
    default:
        if (isprint(key)) {
            insert(m_point_column, m_point_line, static_cast<char>(key));
            m_point_column++;
        } else {
            return false;
        }
        break;
    }
    return true;
}
#endif

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
}

}
