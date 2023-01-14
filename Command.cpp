/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Format.h>

#include <App.h>
#include <Command.h>
#include <Editor.h>
#include <SDLContext.h>

namespace Scratch {

std::map<std::string, Command> Command::s_commands = {
    { "scratch-quit",
        { "scratch-quit", "Quits the editor",
            {},
            [](strings const&) -> void { App::instance().quit(); } } },
    { "open-file",
        { "open-file", "Open file",
            { { "File to open", CommandParameterType::ExistingFilename } },
            [](strings const& args) -> void {
                App::instance().get_component<Editor>()->open_file(args[0]);
            } } },
    { "save-file",
        { "save-file", "Save current file",
            {},
            [](strings const&) -> void { App::instance().get_component<Editor>()->save_file(); } } },
    { "invoke",
        { "invoke", "Invoke command",
            { { "Command", CommandParameterType::Command } },
            [](strings const& args) -> void {
                if (auto const* cmd = Command::get(args[0]); cmd != nullptr) {
                    App::instance().schedule(cmd);
                }
            } } },
};

std::map<SDLKey, std::string> Command::s_key_bindings = {
    { { SDLK_q, KMOD_CTRL }, "scratch-quit" },
    { { SDLK_F2, KMOD_NONE }, "open-file" },
    { { SDLK_F3, KMOD_NONE }, "save-file" },
    { { SDLK_x, KMOD_GUI }, "invoke" },
};

bool Command::bind(SDLKey key)
{
    for (auto const& [bound_key, command] : s_key_bindings) {
        if ((key.sym == bound_key.sym) && ((bound_key.mod == KMOD_NONE && key.mod == KMOD_NONE) || (bound_key.mod & key.mod))) {
            return false;
        }
    }
    s_key_bindings[key] = name;
    return true;
}

std::map<std::string, Command> const& Command::commands()
{
    return s_commands;
}

Command const& Command::register_command(Command cmd)
{
    auto name = cmd.name;
    s_commands[name] = std::move(cmd);
    return s_commands[name];
}

bool Command::is_bound(SDLKey const& key)
{
    return s_key_bindings.contains(key);
}

Command* Command::command_for_key(SDLKey const& key)
{
    if (!is_bound(key))
        return nullptr;
    auto cmd = s_key_bindings.at(key);
    assert(s_commands.contains(cmd));
    return &s_commands.at(cmd);
}

Command* Command::get(std::string const& cmd_name)
{
    if (!s_commands.contains(cmd_name))
        return nullptr;
    return &s_commands.at(cmd_name);
}

AbstractArgumentHandler::AbstractArgumentHandler(CommandHandler* handler, CommandParameter const& parameter, int height)
    : ModalWidget(App::instance().width() / 6, (App::instance().height() - height)/2, App::instance().width() * 0.66, height)
    , m_handler(handler)
    , m_parameter(parameter)
{
}

void AbstractArgumentHandler::submit(std::string const& value)
{
    m_handler->argument_done(value);
}

DefaultArgumentHandler::DefaultArgumentHandler(CommandHandler* handler, CommandParameter const& parameter)
    : AbstractArgumentHandler(handler, parameter, 2 * (char_height + 2) + 8)
{
}

void DefaultArgumentHandler::render()
{
    box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
    rectangle(SDL_Rect { 2, 2, width() - 4, height() - 4 }, { 0xff, 0xff, 0xff, 0xff });
    render_fixed(8, 4, m_parameter.prompt);
    render_fixed(8, char_height + 10, m_value);

    static auto time_start = std::chrono::system_clock::now();
    auto time_end = std::chrono::system_clock::now();
    const long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
    if (elapsed > 400) {
        SDL_Rect r {
            8 + char_width*m_pos,
            char_height + 10,
            1,
            char_height
        };
        box(r, App::instance().color(PaletteIndex::Cursor));
        if (elapsed > 800)
            time_start = time_end;
    }
}

bool DefaultArgumentHandler::dispatch(SDL_Keysym sym)
{
    switch (sym.sym) {
    case SDLK_RETURN: {
        submit(m_value);
        return true;
    };
    case SDLK_LEFT: {
        if (m_pos > 0)
            m_pos--;
        return true;
    }
    case SDLK_RIGHT: {
        if (m_pos < m_value.length() - 1)
            m_pos++;
        return true;
    }
    case SDLK_HOME:
        m_pos = 0;
        return true;
    case SDLK_END:
        m_pos = m_value.length();
        return true;
    case SDLK_BACKSPACE: {
        if (m_pos > 0) {
            m_value.erase(m_pos - 1, 1);
            m_pos--;
        }
        return true;
    }
    case SDLK_UP: {
        // Spin int up
        return true;
    };
    case SDLK_DOWN: {
        // Spin int down
        return true;
    };
    default:
        return false;
    }
}

void DefaultArgumentHandler::handle_text_input()
{
    auto str = App::instance().input_buffer();
    if (str.empty())
        return;
    if (m_pos <= m_value.length()) {
        m_value.insert(m_pos, str);
        m_pos += str.length();
    }
}

FileArgumentHandler::FileArgumentHandler(CommandHandler* handler, CommandParameter const& parameter)
    : AbstractArgumentHandler(handler, parameter, App::instance().height() * 0.66)
    , m_directory(parameter.type == CommandParameterType::ExistingDirectory || parameter.type == CommandParameterType::Directory)
    , m_only_existing(parameter.type == CommandParameterType::ExistingFilename || parameter.type == CommandParameterType::ExistingDirectory)
    , m_lines((height() - char_height - 10) / (char_height + 2))
{
    initialize_dir();
}

void FileArgumentHandler::initialize_dir()
{
    m_entries.clear();
    m_current = 0;
    m_top = 0;
    m_path = fs::canonical(m_path);
    for (auto const& dir_entry : fs::directory_iterator { m_path }) {
        if (dir_entry.path() == fs::path{ "." } )
            continue;
        if (m_directory && !dir_entry.is_directory())
            continue;
        m_entries.push_back(dir_entry);
    }
    std::sort(m_entries.begin(), m_entries.end(), [this](auto const& e1, auto const& e2) -> bool {
        if (!m_directory) {
            if (e1.is_directory() && !e2.is_directory())
                return true;
            if (!e1.is_directory() && e2.is_directory())
                return false;
        }
        auto e1_rel = fs::relative(e1.path());
        auto e2_rel = fs::relative(e2.path());
        return e1_rel.string() < e2_rel.string();
    });
}

void FileArgumentHandler::render()
{
    box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
    rectangle(SDL_Rect { 2, 2, width() - 4, height() - 4 }, { 0xff, 0xff, 0xff, 0xff });
    render_fixed(8, 8, m_parameter.prompt + " [" + m_path.string() + "]");
    auto h = App::instance().context()->character_height();
    auto y = h + 10;
    for (auto count = 0; count < m_entries.size(); count++) {
        if (count < m_top)
            continue;
        if (count >= m_top + m_lines)
            break;
        if (count == m_current) {
            SDL_Rect r {
                4,
                y - 1,
                -4,
                App::instance().context()->character_height() + 1
            };
            box(r, App::instance().color(PaletteIndex::CurrentLineFill));
            rectangle(r, App::instance().color(PaletteIndex::CurrentLineEdge));
        }
        std::string s { m_entries[count].is_directory() ? "+" : " "};
        s += fs::relative(m_entries[count].path()).filename();
        render_fixed(10, y, s);
        y += h + 2;
        if (y > height() - h - 2)
            break;
    }
}

bool FileArgumentHandler::dispatch(SDL_Keysym sym)
{
    switch (sym.sym) {
    case SDLK_RETURN: {
        auto const& e = m_entries[m_current];
        if (!e.is_directory() || m_directory)
            submit(e.path());
        return true;
    };
    case SDLK_LEFT: {
        m_path = m_path.parent_path();
        initialize_dir();
    }
    case SDLK_RIGHT: {
        auto const& e = m_entries[m_current];
        if (e.is_directory()) {
            m_path = m_path / e.path();
            initialize_dir();
        }
        return true;
    }
    case SDLK_UP: {
        if (m_current > 0)
            m_current--;
        return true;
    };
    case SDLK_DOWN: {
        if (m_current < m_entries.size() - 1 && m_current < m_top + m_lines - 1)
            m_current++;
        return true;
    };
    default:
        return false;
    }
}

CommandArgumentHandler::CommandArgumentHandler(CommandHandler* handler, CommandParameter const& parameter)
    : AbstractArgumentHandler(handler, parameter, App::instance().height() * 0.66)
    , m_lines((height() - char_height - 10) / (char_height + 2))
{
}

void CommandArgumentHandler::render()
{
    box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
    rectangle(SDL_Rect { 2, 2, width() - 4, height() - 4 }, { 0xff, 0xff, 0xff, 0xff });
    render_fixed(8, 8, m_parameter.prompt);
    auto y = char_height + 10;
    auto count = 0;
    for (auto const& [ name, cmd ] : Command::commands()) {
        if (count < m_top) {
            count++;
            continue;
        }
        if (count >= m_top + m_lines) {
            break;
        }
        if (count == m_current) {
            SDL_Rect r {
                4,
                y - 1,
                -4,
                char_height + 1
            };
            box(r, App::instance().color(PaletteIndex::CurrentLineFill));
            rectangle(r, App::instance().color(PaletteIndex::CurrentLineEdge));
            m_current_command = cmd.name;
        }
        render_fixed(10, y, Obelix::format("{20} {20}", cmd.name, cmd.synopsis));
        y += char_height + 2;
        if (y > height() - char_height - 2)
            break;
        count++;
    }
}

bool CommandArgumentHandler::dispatch(SDL_Keysym sym)
{
    switch (sym.sym) {
    case SDLK_RETURN: {
        submit(m_current_command);
        return true;
    };
    case SDLK_UP: {
        if (m_current > 0)
            m_current--;
        return true;
    };
    case SDLK_DOWN: {
        if (m_current < Command::commands().size() - 1 && m_current < m_top + m_lines - 1)
            m_current++;
        return true;
    };
    default:
        return false;
    }
}

CommandHandler::CommandHandler(Command const& command)
    : ModalWidget(App::instance().width() / 4, App::instance().height() / 4, App::instance().width() / 2, App::instance().height() / 2)
    , m_command(command)
{
}

void CommandHandler::render()
{
    if (m_current_parameter >= m_command.parameters.size()) {
        m_command.function(m_arguments);
        dismiss();
        return;
    }
    if (m_current_handler == nullptr) {
        auto const& parameter = m_command.parameters[m_current_parameter];
        switch (parameter.type) {
        case CommandParameterType::ExistingFilename:
        case CommandParameterType::Filename:
        case CommandParameterType::Directory:
        case CommandParameterType::ExistingDirectory:
            m_current_handler = new FileArgumentHandler(this, parameter);
            break;
        case CommandParameterType::Command:
            m_current_handler = new CommandArgumentHandler(this, parameter);
            break;
        default:
            m_current_handler = new DefaultArgumentHandler(this, parameter);
            break;
        }
        App::instance().add_modal(m_current_handler);
    }
}

void CommandHandler::argument_done(std::string value)
{
    m_arguments.emplace_back(std::move(value));
    assert(m_current_handler != nullptr);
    m_current_handler->dismiss();
    m_current_handler = nullptr;
    m_current_parameter++;
}

} // Scratch
