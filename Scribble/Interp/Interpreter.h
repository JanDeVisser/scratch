/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Scribble/Context.h>
#include <Scribble/Interp/ExpressionResult.h>
#include <Scribble/Processor.h>
#include <Scribble/Syntax/Statement.h>

namespace Scratch::Interp {

using namespace Obelix;

struct StatementResult {
    enum class StatementResultType {
        None,
        Error,
        Break,
        Continue,
        Return,
    };
    StatementResultType type { StatementResultType::None };
    Value payload {};

    StatementResult& operator=(StatementResult const&) = default;
};

using InterpreterContext = Context<Value,StatementResult>;
[[nodiscard]] ProcessResult interpret(std::shared_ptr<Project> const&);
[[nodiscard]] ProcessResult interpret(std::shared_ptr<Project> const&, InterpreterContext&);

}
