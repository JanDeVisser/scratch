/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <SDL.h>

#include <App/Forward.h>
#include <App/Key.h>

namespace Scratch {

namespace fs=std::filesystem;

enum class CommandParameterType {
    String,
    Integer,
    Filename,
    ExistingFilename,
    Directory,
    ExistingDirectory,
    Command,
    Buffer
};

struct CommandParameter {
    std::string prompt;
    CommandParameterType type { CommandParameterType::String };
    std::function<std::string(void)> get_default { nullptr };
};

using CommandHandlerFunction = std::function<void(Widget&, std::vector<std::string> const&)>;

class Command {
public:
    std::string name;
    std::string synopsis;
    std::vector<CommandParameter> parameters;
    CommandHandlerFunction function;
};

struct ScheduledCommand {
    Widget& owner;
    Command command;
};

class Commands {
public:
    std::map<std::string, Command> const& commands();
    Command const& register_command(Command, SDLKey = { SDLK_UNKNOWN, KMOD_NONE} );
    bool bind(std::string const&, SDLKey);
    bool is_bound(SDLKey const&);
    Command* command_for_key(SDLKey const&);
    Command* get(std::string const&);
    [[nodiscard]] std::vector<Command> operator*();

private:
    std::map<std::string, Command> m_commands;
    std::map<SDLKey, std::string> m_key_bindings;
};

}
