/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lexer/BasicParser.h>
#include <EditorState.h>

using namespace Obelix;

namespace Scratch::Parser {

class ScratchParser : public BasicParser {
public:
    virtual ~ScratchParser() = default;
    [[nodiscard]] virtual Token const& next_token() = 0;
    [[nodiscard]] virtual DisplayToken colorize(TokenCode, std::string_view const&) = 0;
protected:
    ScratchParser() = default;
};

}
