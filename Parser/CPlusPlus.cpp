/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Document.h>
#include <Parser/CPlusPlus.h>

namespace Scratch::Parser {

PlainTextParser::PlainTextParser()
{
    lexer().add_scanner("plaintext", [](Tokenizer& tokenizer) {
        switch (int ch = tokenizer.peek()) {
        case '\n':
            tokenizer.push();
            tokenizer.accept(TokenCode::NewLine);
            break;
        case 0:
            break;
        default:
            do {
                tokenizer.push();
                ch = tokenizer.peek();
            } while (ch && ch != '\n');
            tokenizer.accept(TokenCode::Text);
            break;
        }
    });
}

Token PlainTextParser::next_token()
{
    return lex();
}

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
        Token(KeywordIfndef, "#ifndef"),
        Token(KeywordInclude, "#include"),
        Token(KeywordPragma, "#pragma"));
}

Token CPlusPlusParser::next_token()
{
    Token token;
    if (!m_pending.empty()) {
        token = m_pending.front();
        m_pending.pop_front();
        return token;
    }

    while (m_pending.empty()) {
        token = lex();
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
        case KeywordIfndef:
        case KeywordElifdef:
            m_pending.emplace_back(TokenDirective, token.value());
            parse_ifdef();
            break;

        case KeywordEndif:
        case KeywordHashElse:
            m_pending.emplace_back(TokenDirective, token.value());
            break;

        case KeywordClass:
        case KeywordStruct:
            m_pending.emplace_back(TokenKeyword, token.value());
            token = peek();
            if (token.code() == TokenCode::Identifier) {
                lex();
                m_pending.emplace_back(TokenType, token.value());
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

void CPlusPlusParser::parse_include()
{
    Token t = skip_whitespace();
    switch (t.code()) {
    case Obelix::TokenCode::DoubleQuotedString:
        lex();
        m_pending.emplace_back(TokenDirectiveParam, t.value());
        break;
    case TokenCode::LessThan: {
        lex();
        auto include = t.value();
        do {
            t = lex();
            include += t.value();
        } while (t.code() != Obelix::TokenCode::GreaterThan);
        m_pending.emplace_back(TokenDirectiveParam, include);
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
    while (true) {
        t = peek();
        switch (t.code()) {
        case TokenCode::Comment:
            m_pending.emplace_back(TokenMacroExpansion, def_string);
            return;
        case TokenCode::Backslash: {
            lex();
            escape = !escape;
            def_string += t.value();
            break;
        }
        case TokenCode::NewLine:
            lex();
            m_pending.emplace_back(TokenMacroExpansion, def_string);
            m_pending.emplace_back(TokenCode::NewLine, "\n");
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
    }
}

void CPlusPlusParser::parse_ifdef()
{
    Token t = skip_whitespace();
    if (t.code() != TokenCode::Identifier)
        return;
    lex();
    m_pending.emplace_back(TokenDirectiveParam, t.value());
}

void CPlusPlusParser::parse_hashif()
{
    Token t = skip_whitespace();
    auto escape { false };
    std::string expr;
    while (true) {
        t = peek();
        switch (t.code()) {
        case TokenCode::Comment:
            m_pending.emplace_back(TokenDirectiveParam, expr);
            return;
        case TokenCode::Backslash: {
            lex();
            escape = !escape;
            expr += t.value();
            break;
        }
        case TokenCode::NewLine:
            lex();
            m_pending.emplace_back(TokenDirectiveParam, expr);
            m_pending.emplace_back(TokenCode::NewLine, "\n");
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
    }
}

Token CPlusPlusParser::skip_whitespace()
{
    Token t = peek();
    if (t.code() == TokenCode::Whitespace) {
        t = lex();
        m_pending.emplace_back(t);
        t = peek();
    }
    return t;
}

Token CPlusPlusParser::get_next(TokenCode code)
{
    Token t = lex();
    m_pending.emplace_back((code != TokenCode::Unknown) ? code : t.code(), t.value());
    return skip_whitespace();
}

}
