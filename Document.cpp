/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>
#include <unistd.h>

#include <App.h>
#include <Document.h>

namespace Scratch {

Document::Document()
{
    m_lines.emplace_back();
    m_parser.lexer().add_scanner<Obelix::QStringScanner>();
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
        Token(KeywordLink, "->"),
        Token(KeywordFor, "for"),
        Token(KeywordConst, "const"),
        Token(KeywordStruct, "struct"),
        Token(KeywordStatic, "static"),
        Token(KeywordEnum, "enum"),
        Token(KeywordNamespace, "namespace"),
        Token(KeywordNullptr, "nullptr"),
        TokenCode::BinaryIncrement,
        TokenCode::BinaryDecrement,
        TokenCode::UnaryIncrement,
        TokenCode::UnaryDecrement,
        TokenCode::GreaterEqualThan,
        TokenCode::LessEqualThan,
        TokenCode::EqualsTo,
        TokenCode::NotEqualTo,
        TokenCode::LogicalAnd,
        TokenCode::LogicalOr,
        TokenCode::ShiftLeft,
        TokenCode::ShiftRight);
}

std::string const& Document::line(size_t line_no) const
{
    assert(line_no < m_lines.size());
    return m_lines[line_no];
}

size_t Document::line_length(size_t line_no) const
{
    assert(line_no < m_lines.size());
    return m_lines[line_no].length();
}

size_t Document::line_count() const
{
    return m_lines.size();
}

void Document::backspace(size_t column, size_t row)
{
    assert(row < m_lines.size());
    assert(column <= m_lines[row].length());
    assert(column > 0 || row > 0);
    if (column > 0) {
        m_lines[row].erase(column - 1, 1);
    } else {
        join_lines(row-1);
    }
    invalidate();
}

void Document::split_line(size_t column, size_t row)
{
    if (row >= m_lines.size()) {
        m_lines.emplace_back("");
        invalidate();
        return;
    }
    auto right = m_lines[row].substr(column);
    m_lines[row].erase(column);
    auto iter = m_lines.begin();
    for (auto ix = 0u; ix <= row; ++ix, ++iter);
    m_lines.insert(iter, right);
    invalidate();
}

void Document::join_lines(size_t row)
{
    assert(row < m_lines.size()-1);
    m_lines[row] += m_lines[row+1];
    auto iter = m_lines.begin();
    for (auto ix = 0u; ix < row+1; ++ix, ++iter);
    m_lines.erase(iter);
    invalidate();
}

void Document::insert(size_t column, size_t row, char ch)
{
    assert(row < m_lines.size());
    assert(column <= m_lines[row].length());
    m_lines[row].insert(column, 1, ch);
    invalidate();
}

void Document::clear()
{
    m_lines.clear();
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
    return "";
}

std::string Document::save_as(std::string)
{
    return "";
}

std::string const& Document::filename() const
{
    return m_filename;
}

Token const& Document::lex()
{
    auto& token = m_parser.lex();
    std::string token_text;
    char quote = '\0';
    switch (token.code()) {
    case TokenCode::NewLine:
        m_lines.emplace_back();
        break;
    case TokenCode::SingleQuotedString:
        quote = '\'';
        break;
    case TokenCode::BackQuotedString:
        quote = '`';
        break;
    case TokenCode::DoubleQuotedString:
        quote = '"';
        break;
    default:
        token_text = token.value();
        break;
    }
    if (quote)
        token_text = format("{c}{}{c}", quote, token.code(), quote);
    if (!token_text.empty())
        m_lines.back() += token_text;
    return token;
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
