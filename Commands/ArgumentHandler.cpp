/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Commands/ArgumentHandler.h>

namespace Scratch {

ArgumentHandler::ArgumentHandler(CommandHandler* handler, CommandParameter const& parameter, int height)
    : ModalWidget((int)(App::instance().width() * 0.66), height)
    , m_handler(handler)
    , m_parameter(parameter)
{
}

/*
 * fs::directory_entry - for File/Directory parameters
 */

template<>
std::string line_text(fs::directory_entry const& entry, fs::path const&)
{
    return fs::relative(entry.path());
}

template<>
void submit(CommandHandler* handler, fs::directory_entry const& entry, fs::path const&)
{
    handler->argument_done(fs::absolute(entry).string());
}

template<>
std::vector<fs::directory_entry> get_entries(CommandParameter const& param, fs::path const& path)
{
    std::vector<fs::directory_entry> entries;
    fs::path p { path };
    if (p.empty())
        p = fs::current_path();
    auto canon_path = fs::canonical(p);
    bool only_dirs = param.type == CommandParameterType::Directory || param.type == CommandParameterType::ExistingDirectory;
    for (auto const& dir_entry : fs::directory_iterator { canon_path }) {
        if (dir_entry.path() == fs::path { "." })
            continue;
        if (only_dirs && !dir_entry.is_directory())
            continue;
        entries.push_back(dir_entry);
    }
    std::sort(entries.begin(), entries.end(), [](auto const& e1, auto const& e2) -> bool {
        if (e1.is_directory() && !e2.is_directory())
            return true;
        if (!e1.is_directory() && e2.is_directory())
            return false;
        auto e1_rel = fs::relative(e1.path());
        auto e2_rel = fs::relative(e2.path());
        return e1_rel.string() < e2_rel.string();
    });
    return entries;
}

template<>
bool handle(ListArgumentHandler<fs::directory_entry, fs::path>* handler, fs::directory_entry const& entry, fs::path const& path, SDL_Keysym key)
{
    switch (key.sym) {
    case SDLK_RETURN: {
        if (entry.is_directory() && handler->parameter().type != CommandParameterType::Directory && handler->parameter().type != CommandParameterType::ExistingDirectory) {
            handler->set_entries(path / entry.path());
            return true;
        }
    } break;
    case SDLK_LEFT: {
        handler->set_entries(path.parent_path());
        return true;
    }
    case SDLK_RIGHT: {
        if (entry.is_directory()) {
            handler->set_entries(path / entry.path());
            return true;
        }
    } break;
    default:
        break;
    }
    return false;
}


/*
 * Command - For Command parameters
 */

template<>
std::string line_text(Command const& entry, int const&)
{
    return Obelix::format("{20} {20}", entry.name, entry.synopsis);
}

template<>
void submit(CommandHandler* handler, Command const& entry, int const&)
{
    handler->argument_done(entry.name);
}

template<>
std::vector<Command> get_entries(CommandParameter const& param, int const&)
{
    std::vector<Command> entries;
    for (auto cmd : App::instance().commands()) {
        entries.push_back(cmd.command);
    }
    std::sort(entries.begin(), entries.end(), [](auto const& e1, auto const& e2) -> bool {
        return e1.name < e2.name;
    });
    return entries;
}


/*
 * fs::path - for Buffer parameters
 */

template<>
std::string line_text(fs::path const& entry, int const&)
{
    return fs::relative(entry);
}

template<>
void submit(CommandHandler* handler, fs::path const& entry, int const&)
{
    handler->argument_done(fs::absolute(entry).string());
}

template<>
std::vector<fs::path> get_entries(CommandParameter const& param, int const&)
{
    std::vector<fs::path> entries;
    for (auto const* doc : Scratch::editor()->documents()) {
        entries.push_back(doc->path());
    }
    std::sort(entries.begin(), entries.end(), [](auto const& e1, auto const& e2) -> bool {
        return e1.string() < e2.string();
    });
    return entries;
}

ModalWidget* create_argument_handler(CommandHandler* handler, CommandParameter const& parameter)
{
    switch (parameter.type) {
    case CommandParameterType::ExistingFilename:
    case CommandParameterType::Filename:
    case CommandParameterType::Directory:
    case CommandParameterType::ExistingDirectory:
        return new ListArgumentHandler<fs::directory_entry, fs::path>(handler, parameter, fs::current_path());
    case CommandParameterType::Command:
        return new ListArgumentHandler<Command>(handler, parameter, 0);
    case CommandParameterType::Buffer:
        return new ListArgumentHandler<fs::path>(handler, parameter, 0);
    default:
        return new DefaultArgumentHandler(handler, parameter);
    }
}


}
