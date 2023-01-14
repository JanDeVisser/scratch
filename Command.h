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

class AbstractArgumentHandler : public ModalWidget {
protected:
    AbstractArgumentHandler(CommandHandler*, CommandParameter const&, int);
    void submit(std::string const&);

    CommandHandler* m_handler;
    CommandParameter const& m_parameter;
};

class DefaultArgumentHandler : public AbstractArgumentHandler {
public:
    DefaultArgumentHandler(CommandHandler*, CommandParameter const&);
    void render() override;
    bool dispatch(SDL_Keysym) override;
    void handle_text_input() override;
private:
    std::string m_value;
    int m_pos { 0 };
};

class FileArgumentHandler : public AbstractArgumentHandler {
public:
    FileArgumentHandler(CommandHandler*, CommandParameter const&);
    void render() override;
    bool dispatch(SDL_Keysym) override;
private:
    void initialize_dir();

    bool m_directory;
    bool m_only_existing;
    int m_lines;
    fs::path m_path { "." };
    int m_top { 0 };
    int m_current { 0 };
    std::vector<fs::directory_entry> m_entries;
};

class CommandArgumentHandler : public AbstractArgumentHandler {
public:
    CommandArgumentHandler(CommandHandler*, CommandParameter const&);
    void render() override;
    bool dispatch(SDL_Keysym) override;
private:
    int m_lines;
    int m_top { 0 };
    int m_current { 0 };
    std::string m_current_command;
};

class CommandHandler : public ModalWidget {
public:
    explicit CommandHandler(Command const& command);
    void render() override;
    void argument_done(std::string);
private:
    Command const& m_command;
    ModalWidget* m_current_handler { nullptr };
    std::vector<std::string> m_arguments;
    int m_current_parameter { 0 };
};

}
