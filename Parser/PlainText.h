/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lexer/Token.h>
#include <App/EditorState.h>
#include <Parser/ScratchParser.h>

namespace Scratch::Parser {

class PlainTextParser : public ScratchParser {
public:
    PlainTextParser();
    Token const& next_token() override;
    DisplayToken colorize(TokenCode, std::string_view const&) override;

private:
    std::string m_current_line;
};

}
