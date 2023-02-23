/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Parser/CPlusPlus.h>

namespace Scratch::Parser {

CPlusPlusParser::CPlusPlusParser()
{
    lexer().add_scanner<Obelix::QStringScanner>("\"'", true);
    lexer().add_scanner<Obelix::IdentifierScanner>();
    lexer().add_scanner<Obelix::NumberScanner>(Obelix::NumberScanner::Config { true, false, true, false, true });
    lexer().add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    lexer().add_scanner<Obelix::CommentScanner>(true,
        Obelix::CommentScanner::CommentMarker { false, false, "/*", "*/" },
        Obelix::CommentScanner::CommentMarker { false, true, "//", "" });
    lexer().add_scanner<Obelix::KeywordScanner>(
        KeywordAuto, "auto",
        KeywordBreak, "break",
        KeywordCase, "case",
        KeywordClass, "class",
        KeywordConst, "const",
        KeywordContinue, "continue",
        KeywordDefault, "default",
        KeywordElse, "else",
        KeywordEnum, "enum",
        KeywordFor, "for",
        KeywordIf, "if",
        KeywordNamespace, "namespace",
        KeywordReturn, "return",
        KeywordStatic, "static",
        KeywordStruct, "struct",
        KeywordSwitch, "switch",
        KeywordUsing, "using",
        KeywordWhile, "while",

        KeywordTrue, "true",
        KeywordFalse, "false",
        KeywordNullptr, "nullptr",

        KeywordDefine, "#define",
        KeywordElif, "#elif",
        KeywordElifdef, "#elifdef",
        KeywordHashElse, "#else",
        KeywordEndif, "#endif",
        KeywordHashIf, "#if",
        KeywordIfdef, "#ifdef",
        KeywordIfndef, "#ifndef",
        KeywordInclude, "#include",
        KeywordPragma, "#pragma");
}

DisplayToken CPlusPlusParser::colorize(TokenCode code, std::string_view const& text)
{
    PaletteIndex color;
    switch (code) {
    case TokenCode::Comment:
        color = PaletteIndex::Comment;
        break;
    case TokenCode::Identifier:
        color = PaletteIndex::Identifier;
        break;
    case TokenCode::DoubleQuotedString:
        color = PaletteIndex::CharLiteral;
        break;
    case TokenCode::SingleQuotedString:
        color = PaletteIndex::String;
        break;
    case TokenKeyword:
    case TokenConstant:
        color = PaletteIndex::Keyword;
        break;
    case TokenDirective:
        color = PaletteIndex::Preprocessor;
        break;
    default:
        color = PaletteIndex::Punctuation;
        break;
    }
    return DisplayToken { text, color };
}

Token const& CPlusPlusParser::next_token()
{
    if (!m_pending.empty()) {
        auto const& token = m_pending.front();
        m_pending.pop_front();
        return token;
    }

    while (m_pending.empty()) {
        auto const& token = lex();
        switch (token.code()) {
        case TokenCode::NewLine:
            m_pending.emplace_back(token);
            break;

        case KeywordInclude:
            m_pending.emplace_back(token.location(), TokenDirective, token.value());
            parse_include();
            break;

        case KeywordDefine:
            m_pending.emplace_back(token.location(), TokenDirective, token.value());
            parse_define();
            break;

        case KeywordHashIf:
        case KeywordElif:
        case KeywordPragma:
            m_pending.emplace_back(token.location(), TokenDirective, token.value());
            parse_hashif();
            break;

        case KeywordIfdef:
        case KeywordIfndef:
        case KeywordElifdef:
            m_pending.emplace_back(token.location(), TokenDirective, token.value());
            parse_ifdef();
            break;

        case KeywordEndif:
        case KeywordHashElse:
            m_pending.emplace_back(token.location(), TokenDirective, token.value());
            break;

        case KeywordClass:
        case KeywordStruct:
            m_pending.emplace_back(token.location(), TokenKeyword, token.value());
            if (peek().code() == TokenCode::Identifier) {
                auto const& identifier = lex();
                m_pending.emplace_back(identifier.location(), TokenType, identifier.value());
            }
            break;

        case KeywordAuto:
        case KeywordConst:
        case KeywordIf:
        case KeywordElse:
        case KeywordNamespace:
        case KeywordWhile:
        case KeywordEnum:
        case KeywordFor:
        case KeywordReturn:
        case KeywordSwitch:
        case KeywordCase:
        case KeywordBreak:
        case KeywordContinue:
        case KeywordDefault:
        case KeywordStatic:
        case KeywordUsing:
            m_pending.emplace_back(token.location(), TokenKeyword, token.value());
            break;

        case KeywordNullptr:
        case KeywordTrue:
        case KeywordFalse:
            m_pending.emplace_back(token.location(), TokenConstant, token.value());
            break;

        default:
            m_pending.emplace_back(token);
            break;
        }
    }

    auto const& token = m_pending.front();
    m_pending.pop_front();
    return token;
}

void CPlusPlusParser::parse_include()
{
    Token t = skip_whitespace();
    switch (t.code()) {
    case Obelix::TokenCode::DoubleQuotedString:
        lex();
        m_pending.emplace_back(t.location(), TokenDirectiveParam, t.value());
        break;
    case TokenCode::LessThan: {
        lex();
        auto include = std::string(t.value());
        auto start_loc = t.location();
        Span end_loc;
        do {
            t = lex();
            include += t.value();
            end_loc = t.location();
        } while (t.code() != Obelix::TokenCode::GreaterThan);
        m_pending.emplace_back(start_loc.merge(end_loc), TokenDirectiveParam, include);
        break;
    }
    default:
        break;
    }
}

void CPlusPlusParser::parse_define()
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
                lex();
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
    t = peek();
    auto start_loc = t.location();
    auto end_loc = start_loc;
    while (true) {
        switch (t.code()) {
        case TokenCode::Comment:
            m_pending.emplace_back(start_loc.merge(end_loc), TokenMacroExpansion, def_string);
            return;
        case TokenCode::Backslash: {
            lex();
            escape = !escape;
            def_string += t.value();
            break;
        }
        case TokenCode::NewLine:
            lex();
            m_pending.emplace_back(start_loc.merge(end_loc), TokenMacroExpansion, def_string);
            m_pending.emplace_back(t.location(), TokenCode::NewLine, "\n");
            if (!escape)
                return;
            def_string = "";
            escape = false;
            break;
        default:
            escape = false;
            lex();
            def_string += t.value();
            break;
        }
        t = peek();
        end_loc = t.location();
    }
}

void CPlusPlusParser::parse_ifdef()
{
    Token t = skip_whitespace();
    if (t.code() != TokenCode::Identifier)
        return;
    lex();
    m_pending.emplace_back(t.location(), TokenDirectiveParam, t.value());
}

void CPlusPlusParser::parse_hashif()
{
    Token t = skip_whitespace();
    auto escape { false };
    std::string expr;
    t = peek();
    auto start_loc = t.location();
    auto end_loc = t.location();
    while (true) {
        switch (t.code()) {
        case TokenCode::Comment:
            m_pending.emplace_back(start_loc.merge(end_loc), TokenDirectiveParam, expr);
            return;
        case TokenCode::Backslash: {
            lex();
            escape = !escape;
            expr += t.value();
            break;
        }
        case TokenCode::NewLine:
            lex();
            m_pending.emplace_back(start_loc.merge(end_loc), TokenDirectiveParam, expr);
            m_pending.emplace_back(t.location(), TokenCode::NewLine, "\n");
            if (!escape)
                return;
            expr = "";
            escape = false;
            break;
        default:
            escape = false;
            lex();
            expr += t.value();
            break;
        }
        t = peek();
        end_loc = t.location();
    }
}

Token const& CPlusPlusParser::skip_whitespace()
{
    if (peek().code() != TokenCode::Whitespace)
        return peek();
    m_pending.emplace_back(lex());
    return m_pending.back();
}

Token const& CPlusPlusParser::get_next(TokenCode code)
{
    Token t = lex();
    m_pending.emplace_back(t.location(), (code != TokenCode::Unknown) ? code : t.code(), t.value());
    return skip_whitespace();
}

}
