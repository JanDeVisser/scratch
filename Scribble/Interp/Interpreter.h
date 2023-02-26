/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Scribble/Interp/ExpressionResult.h>
#include <Scribble/Processor.h>
#include <Scribble/Syntax/Statement.h>

namespace Scratch::Interp {

using namespace Obelix;

[[nodiscard]] ProcessResult interpret(Project const&);

}
