/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Parser/PlainText.h>

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

Token const& PlainTextParser::next_token()
{
    return lex();
}

DisplayToken PlainTextParser::colorize(TokenCode, std::string_view const& text)
{
    return DisplayToken { text, PaletteIndex::Default };
}

}
