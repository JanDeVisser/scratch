/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App/Scratch.h>
#include <Scribble/Interp/CommandAdapter.h>

namespace Scratch::Interp {

CommandAdapter::CommandAdapter(std::string function, ScheduledCommand const& command)
    : Function(std::move(function))
    , m_command(command)
{
}

Value CommandAdapter::execute(std::vector<Value> const& arguments, InterpreterContext&) const
{
    strings args;
    if (arguments.size() < m_command.command.parameters.size())
        return Value {};
    for (auto const& a : arguments) {
        args.push_back(a.to_string());
    }
    m_command.command.function(m_command.owner, args);
    return Value {};
}

} // Scratch::Interp
