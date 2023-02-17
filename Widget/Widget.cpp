/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <SDLContext.h>
#include <Widget/Widget.h>

namespace Scratch {

void Widget::render()
{
}

void Widget::resize(Box const&)
{
}

bool Widget::dispatch(SDL_Keysym sym)
{
    if (m_commands != nullptr) {
        if (auto* cmd = m_commands->command_for_key(sym); cmd != nullptr) {
            App::instance().schedule({ const_cast<Widget&>(*this), *cmd });
            return true;
        }
    }
    return false;
}

std::optional<ScheduledCommand> Widget::command(std::string const& name) const
{
    if (m_commands != nullptr) {
        if (auto* cmd = m_commands->get(name); cmd != nullptr)
            return ScheduledCommand { const_cast<Widget&>(*this), *cmd };
    }
    return {};
}

std::vector<Command> Widget::commands() const
{
    std::vector<Command> ret;
    if (m_commands != nullptr) {
        for (auto const& [ name, cmd ] : m_commands->commands())
            ret.push_back(cmd);
    }
    return ret;
}

}
