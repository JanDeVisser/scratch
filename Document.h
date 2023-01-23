/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <deque>
#include <filesystem>

#include <lexer/BasicParser.h>

#include <Forward.h>

namespace Scratch {

namespace fs=std::filesystem;

using TokenCode=Obelix::TokenCode;
using Token=Obelix::Token;

constexpr static TokenCode KeywordIf = TokenCode::Keyword0;
constexpr static TokenCode KeywordElse = TokenCode::Keyword1;
constexpr static TokenCode KeywordWhile = TokenCode::Keyword2;
constexpr static TokenCode KeywordTrue = TokenCode::Keyword3;
constexpr static TokenCode KeywordFalse = TokenCode::Keyword4;
constexpr static TokenCode KeywordReturn = TokenCode::Keyword5;
constexpr static TokenCode KeywordBreak = TokenCode::Keyword6;
constexpr static TokenCode KeywordContinue = TokenCode::Keyword7;
constexpr static TokenCode KeywordSwitch = TokenCode::Keyword8;
constexpr static TokenCode KeywordCase = TokenCode::Keyword9;
constexpr static TokenCode KeywordDefault = TokenCode::Keyword10;
constexpr static TokenCode KeywordFor = TokenCode::Keyword11;
constexpr static TokenCode KeywordConst = TokenCode::Keyword12;
constexpr static TokenCode KeywordStruct = TokenCode::Keyword13;
constexpr static TokenCode KeywordStatic = TokenCode::Keyword14;
constexpr static TokenCode KeywordEnum = TokenCode::Keyword15;
constexpr static TokenCode KeywordNamespace = TokenCode::Keyword16;
constexpr static TokenCode KeywordNullptr = TokenCode::Keyword17;
constexpr static TokenCode KeywordClass = TokenCode::Keyword18;

constexpr static TokenCode KeywordInclude = TokenCode::Keyword19;
constexpr static TokenCode KeywordDefine = TokenCode::Keyword20;
constexpr static TokenCode KeywordIfdef = TokenCode::Keyword21;
constexpr static TokenCode KeywordEndif = TokenCode::Keyword22;
constexpr static TokenCode KeywordElif = TokenCode::Keyword23;
constexpr static TokenCode KeywordElifdef = TokenCode::Keyword24;
constexpr static TokenCode KeywordPragma = TokenCode::Keyword25;
constexpr static TokenCode KeywordHashIf = TokenCode::Keyword26;
constexpr static TokenCode KeywordHashElse = TokenCode::Keyword27;

constexpr static TokenCode TokenKeyword = TokenCode::Keyword28;
constexpr static TokenCode TokenMacroName = TokenCode::Keyword29;
constexpr static TokenCode TokenMacroParam = TokenCode::Keyword30;
constexpr static TokenCode TokenMacroExpansion = TokenCode::Keyword31;
constexpr static TokenCode TokenDirective = TokenCode::Keyword32;
constexpr static TokenCode TokenDirectiveParam = TokenCode::Keyword33;
constexpr static TokenCode TokenType = TokenCode::Keyword34;
constexpr static TokenCode TokenOperator = TokenCode::Keyword35;
constexpr static TokenCode TokenConstant = TokenCode::Keyword36;

struct Line {
    Line() = default;
    explicit Line(std::string t)
        : text(std::move(t))
    {
    }

    std::string text;
    std::vector<Token> tokens {};
};

class Document {
public:
    explicit Document(Editor *);

    [[nodiscard]] std::string const& line(size_t) const;
    [[nodiscard]] size_t line_length(size_t) const;
    [[nodiscard]] size_t line_count() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t parsed() const;
    [[nodiscard]] fs::path const& path() const;

    [[nodiscard]] int screen_top() const { return m_screen_top; }
    [[nodiscard]] int screen_left() const { return m_screen_left; }
    [[nodiscard]] int point_line() const { return m_point_line; }
    [[nodiscard]] int point_column() const { return m_point_column; }
    [[nodiscard]] int virtual_point_column() const { return m_virtual_point_column; }

    void backspace();
    void split_line();
    void insert(std::string const&);
    void join_lines();
    void move_to(int line, int column);

    void clear();
    std::string load(std::string const&);
    std::string save();
    std::string save_as(std::string const&);
    [[nodiscard]] bool dirty() const { return m_dirty; }

    void render(Editor *editor);
    bool dispatch(Editor *editor, SDL_Keysym);
    void handle_text_input();

    Token lex();
    void rewind();
    void invalidate();

private:
    void parse_include();
    void parse_define();
    void parse_hashif();
    void parse_ifdef();

    Token skip_whitespace();
    Token get_next(TokenCode = TokenCode::Unknown);

    void assign_to_parser();

    Editor* m_editor;
    fs::path m_path {};
    bool m_dirty { false };
    bool m_cleared { true };
    Obelix::BasicParser m_parser;
    std::deque<Token> m_pending;

    std::vector<Line> m_lines {};

    size_t m_screen_top {0};
    size_t m_screen_left {0};
    size_t m_point_line {0};
    size_t m_point_column {0};
    size_t m_virtual_point_column {0};

};

}
