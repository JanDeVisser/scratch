/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lexer/BasicParser.h>

#include <Forward.h>

namespace Scratch {

using TokenCode=Obelix::TokenCode;
using Token=Obelix::Token;

constexpr static TokenCode KeywordIf = TokenCode::Keyword2;
constexpr static TokenCode KeywordElse = TokenCode::Keyword3;
constexpr static TokenCode KeywordWhile = TokenCode::Keyword4;
constexpr static TokenCode KeywordTrue = TokenCode::Keyword5;
constexpr static TokenCode KeywordFalse = TokenCode::Keyword6;
constexpr static TokenCode KeywordReturn = TokenCode::Keyword7;
constexpr static TokenCode KeywordBreak = TokenCode::Keyword8;
constexpr static TokenCode KeywordContinue = TokenCode::Keyword9;
constexpr static TokenCode KeywordSwitch = TokenCode::Keyword11;
constexpr static TokenCode KeywordCase = TokenCode::Keyword12;
constexpr static TokenCode KeywordDefault = TokenCode::Keyword13;
constexpr static TokenCode KeywordLink = TokenCode::Keyword14;
constexpr static TokenCode KeywordFor = TokenCode::Keyword16;
constexpr static TokenCode KeywordConst = TokenCode::Keyword22;
constexpr static TokenCode KeywordStruct = TokenCode::Keyword24;
constexpr static TokenCode KeywordStatic = TokenCode::Keyword25;
constexpr static TokenCode KeywordEnum = TokenCode::Keyword26;
constexpr static TokenCode KeywordNamespace = TokenCode::Keyword27;
constexpr static TokenCode KeywordNullptr = TokenCode::Keyword28;

class Document {
public:
    Document();

    [[nodiscard]] std::string const& line(size_t) const;
    [[nodiscard]] size_t line_length(size_t) const;
    [[nodiscard]] size_t line_count() const;
    [[nodiscard]] std::string const& filename() const;

    void backspace(size_t column, size_t row);
    void split_line(size_t column, size_t row);
    void insert(size_t column, size_t row, char);
    void join_lines(size_t);

    void clear();
    std::string load(std::string);
    std::string save();
    std::string save_as(std::string);
    [[nodiscard]] bool dirty() const { return m_dirty; }

    Token const& lex();
    void rewind();
    void invalidate();

private:
    std::string m_filename;
    strings m_lines;
    bool m_dirty { false };
    bool m_cleared { true };
    Obelix::BasicParser m_parser;
};

}
