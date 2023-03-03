/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Format.h>
#include <core/StringUtil.h>

#include "Widget/SDLContext.h"
#include <App/Scratch.h>
#include <Commands/Command.h>

using namespace Obelix;

namespace Scratch {

class Widget;

bool Commands::bind(std::string const& name, SDLKey key)
{
    for (auto const& [bound_key, command] : m_key_bindings) {
        if ((key.sym == bound_key.sym) && ((bound_key.mod == KMOD_NONE && key.mod == KMOD_NONE) || (bound_key.mod & key.mod))) {
            return false;
        }
    }
    m_key_bindings[key] = name;
    return true;
}

std::map<std::string, Command> const& Commands::commands()
{
    return m_commands;
}

Command const& Commands::register_command(Command cmd, SDLKey key)
{
    auto name = cmd.name;
    m_commands[name] = std::move(cmd);
    if (key.sym != SDLK_UNKNOWN)
        bind(name, key);
    return m_commands[name];
}

bool Commands::is_bound(SDLKey const& key)
{
    return m_key_bindings.contains(key);
}

Command* Commands::command_for_key(SDLKey const& key)
{
    if (!is_bound(key))
        return nullptr;
    auto cmd = m_key_bindings.at(key);
    assert(m_commands.contains(cmd));
    return &m_commands.at(cmd);
}

Command* Commands::get(std::string const& cmd_name)
{
    if (!m_commands.contains(cmd_name))
        return nullptr;
    return &m_commands.at(cmd_name);
}

std::vector<Command> Commands::operator*()
{
    std::vector<Command> ret;
    for (auto const& [ name, cmd ] : m_commands)
        ret.push_back(cmd);
    return ret;
}

} // Scratch
