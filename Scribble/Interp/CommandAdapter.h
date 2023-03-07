/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Commands/Command.h>
#include <Scribble/Interp/Function.h>
#include <Scribble/Interp/Value.h>

namespace Scratch::Interp {

class CommandAdapter : public Function {
public:
    CommandAdapter(std::string, ScheduledCommand const&);
    Value execute(std::vector<Value> const&, InterpreterContext&) const override;
private:
    ScheduledCommand const& m_command;
};

} // Scratch::Interp
