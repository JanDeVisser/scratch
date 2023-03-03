/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lexer/BasicParser.h>
#include <App/EditorState.h>
#include <Commands/Command.h>

using namespace Obelix;

namespace Scratch::Parser {

class ScratchParser : public BasicParser {
public:
    [[nodiscard]] virtual Token const& next_token() = 0;
    [[nodiscard]] virtual DisplayToken colorize(TokenCode, std::string_view const&) = 0;
    [[nodiscard]] virtual std::vector<Command> commands() const;
    [[nodiscard]] virtual std::optional<ScheduledCommand> command(std::string const&) const;

protected:
    ScratchParser() = default;
};

}
