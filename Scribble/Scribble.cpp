/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Scribble.h>

namespace Scratch::Scribble {

Scribble::Scribble()
{
    lexer().add_scanner<Obelix::QStringScanner>("\"'", true);
    lexer().add_scanner<Obelix::IdentifierScanner>();
    lexer().add_scanner<Obelix::NumberScanner>(Obelix::NumberScanner::Config { true, false, true, false, true });
    lexer().add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    lexer().add_scanner<Obelix::CommentScanner>(true,
        Obelix::CommentScanner::CommentMarker { false, false, "/*", "*/" },
        Obelix::CommentScanner::CommentMarker { false, true, "//", "" });
    lexer().add_scanner<Obelix::KeywordScanner>(
        KeywordBreak, "break",
        KeywordCase, "case",
        KeywordCmd, "cmd",
        KeywordConst, "const",
        KeywordContinue, "continue",
        KeywordDecEquals, "-=",
        KeywordDefault, "default",
        KeywordElse, "else",
        KeywordElif, "elif",
        KeywordFor, "for",
        KeywordFor, "func",
        KeywordIf, "if",
        KeywordImport, "import",
        KeywordIn, "in",
        KeywordIncEquals, "+=",
        KeywordIntrinsic, "intrinsic",
        KeywordLink, "->",
        KeywordRange, "..",
        KeywordReturn, "return",
        KeywordSwitch, "switch",
        KeywordVar, "var",
        KeywordWhile, "while",

        KeywordTrue, "true",
        KeywordFalse, "false"
    );
}

DisplayToken Scribble::colorize(TokenCode code, std::string_view const& text)
{
    PaletteIndex color = PaletteIndex::Default;
    if (code >= TokenCode::Keyword0 && code <= TokenCode::Keyword30) {
        color = PaletteIndex::Keyword;
    } else {
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
        case KeywordTrue:
        case KeywordFalse:
        case TokenCode::Integer:
        case TokenCode::Float:
            color = PaletteIndex::Number;
            break;
        default:
            color = PaletteIndex::Punctuation;
            break;
        }
    }
    return DisplayToken { text, color };
}

Token const& Scribble::next_token()
{
    return lex();
}

} // Scratch
