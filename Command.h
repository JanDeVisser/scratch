/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <SDL.h>

#include <Key.h>
#include <Widget.h>

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
    CommandParameterType type;
};

struct Command {
    std::string name;
    std::string synopsis;
    std::vector<CommandParameter> parameters;
    std::function<void(std::vector<std::string> const&)> function;

    bool bind(SDLKey);

    static std::map<std::string, Command> const& commands();
    static Command const& register_command(Command);
    static bool is_bound(SDLKey const&);
    static Command* command_for_key(SDLKey const&);
    static Command* get(std::string const&);

private:
    static std::map<std::string, Command> s_commands;
    static std::map<SDLKey, std::string> s_key_bindings;
};

class CommandHandler;

class CommandHandler : public ModalWidget {
public:
    explicit CommandHandler(Command const& command);
    void render() override;
    void argument_done(std::string);
    void abort();
private:
    Command const& m_command;
    ModalWidget* m_current_handler { nullptr };
    std::vector<std::string> m_arguments;
    int m_current_parameter { 0 };
};

}
