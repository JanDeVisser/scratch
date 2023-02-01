/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Format.h>
#include <core/StringUtil.h>

#include <Command.h>
#include <SDLContext.h>
#include <Scratch.h>

using namespace Obelix;

namespace Scratch {

std::map<std::string, Command> Command::s_commands = {
    { "copy-to-clipboard",
        { "copy-to-clipboard", "Copy selection to clipboard", {},
        [](strings const&) -> void {
                auto doc = Scratch::editor()->document();
                doc->copy_to_clipboard();
            }
        }
    },
    { "cut-to-clipboard",
        { "cut-to-clipboard", "Cut selection to clipboard", {},
        [](strings const&) -> void {
                auto doc = Scratch::editor()->document();
                doc->cut_to_clipboard();
            }
        }
    },
    {
        "enlarge-font",
        { "enlarge-font", "Enlarge editor font", {},
            [](strings const&) -> void {
                auto& app = App::instance();
                app.enlarge_font();
            }
        }
    },
    { "goto-line-column",
        { "goto-line-column", "Goto line:column",
            { { "Line:Column to go to", CommandParameterType::String } },
            [](strings const& args) -> void {
                auto line_col = split(args[0], ':');
                if (line_col.size() < 1)
                    return;
                if (auto line_maybe = to_long(line_col[0]); line_maybe.has_value()) {
                    int col = -1;
                    if (line_col.size() > 1) {
                        if (auto col_maybe = to_long(line_col[1]); col_maybe.has_value())
                            col = col_maybe.value();
                    }
                    Scratch::editor()->move_to(line_maybe.value() - 1, col - 1);
                }
            }
        }
    },
    { "paste-from-clipboard",
        { "paste-from-clipboard", "Paste text from clipboard", {},
        [](strings const&) -> void {
                auto doc = Scratch::editor()->document();
                doc->paste_from_clipboard();
            }
        }
    },
    { "scratch-quit",
        { "scratch-quit", "Quits the editor",
            {},
            [](strings const&) -> void { App::instance().quit(); } } },
    { "new-buffer",
        { "new-buffer", "New buffer",
            {},
            [](strings const&) -> void {
                Scratch::editor()->new_buffer();
            } } },
    { "open-file",
        { "open-file", "Open file",
            { { "File to open", CommandParameterType::ExistingFilename } },
            [](strings const& args) -> void {
                Scratch::editor()->open_file(args[0]);
            } } },
    {
        "reset-font",
        { "reset-font", "Reset editor font", {},
            [](strings const&) -> void {
                auto& app = App::instance();
                app.reset_font();
            }
        }
    },
    { "save-current-as",
        { "save-current-as", "Save current file as",
            { { "New file name", CommandParameterType::String } },
            [](strings const& args) -> void { Scratch::editor()->save_file_as(args[0]); } } },
    { "save-all-files",
        { "save-all-files", "Save call files",
            {},
            [](strings const&) -> void { Scratch::editor()->save_all(); } } },
    { "save-file",
        { "save-file", "Save current file",
            {},
            [](strings const&) -> void {
                auto editor = Scratch::editor();
                assert(editor != nullptr);
                if (editor->document()->path().empty()) {
                    App::instance().schedule(Command::get("save-current-as"));
                } else {
                    editor->save_file();
                }
            } } },
    {
        "shrink-font",
        { "shrink-font", "Shrink editor font", {},
            [](strings const&) -> void {
                auto& app = App::instance();
                app.shrink_font();
            }
        }
    },
    { "switch-buffer",
        { "switch-buffer", "Switch buffer",
            { { "Buffer", CommandParameterType::Buffer } },
            [](strings const& args) -> void {
                Scratch::editor()->switch_to(args[0]);
            } } },
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
    { { SDLK_EQUALS, KMOD_GUI }, "enlarge-font" },
    { { SDLK_MINUS, KMOD_GUI }, "shrink-font" },
    { { SDLK_0, KMOD_GUI }, "reset-font" },
    { { SDLK_b, KMOD_CTRL }, "switch-buffer" },
    { { SDLK_c, KMOD_CTRL }, "copy-to-clipboard" },
    { { SDLK_g, KMOD_CTRL }, "goto-line-column" },
    { { SDLK_l, KMOD_CTRL }, "save-all-files" },
    { { SDLK_n, KMOD_CTRL }, "new-buffer" },
    { { SDLK_o, KMOD_CTRL }, "open-file" },
    { { SDLK_q, KMOD_CTRL }, "scratch-quit" },
    { { SDLK_s, KMOD_CTRL }, "save-file" },
    { { SDLK_s, KMOD_CTRL | KMOD_GUI }, "save-current-as" },
    { { SDLK_v, KMOD_CTRL }, "paste-from-clipboard" },
    { { SDLK_x, KMOD_CTRL }, "cut-to-clipboard" },
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

class AbstractArgumentHandler : public ModalWidget {
protected:
    AbstractArgumentHandler(CommandHandler*, CommandParameter const&, int);

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

template<class EntryCls, typename ContextCls>
class ListArgumentHandler;

template<class EntryCls, typename ContextCls = int>
std::string line_text(EntryCls const& entry, ContextCls const&)
{
    return entry.to_string();
}

template<class EntryCls, typename ContextCls = int>
void submit(CommandHandler* handler, EntryCls const& entry, ContextCls const&)
{
    handler->argument_done(entry.to_string());
}

template<class EntryCls, typename ContextCls = int>
bool handle(ListArgumentHandler<EntryCls, ContextCls>*, EntryCls const&, ContextCls const&, SDL_Keysym)
{
    return false;
}

template<class EntryCls, typename ContextCls = int>
std::vector<EntryCls> get_entries(CommandParameter const&, ContextCls const&)
{
    fatal("get_entries unimplemented for EntryCls '{}' and ContextCls '{}'",
        typeid(EntryCls).name(), typeid(ContextCls).name());
}

template<typename ContextCls = int>
std::string line_text(std::string const& entry, ContextCls const&)
{
    return entry;
}

template<typename ContextCls = int>
void submit(CommandHandler* handler, std::string const& entry, ContextCls const&)
{
    handler->argument_done(entry);
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
    auto canon_path = fs::canonical(path);
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
    for (auto const& [name, cmd] : Command::commands()) {
        entries.push_back(cmd);
    }
    std::sort(entries.begin(), entries.end(), [](auto const& e1, auto const& e2) -> bool {
        return e1.name < e2.name;
    });
    return entries;
}

/* ----------------------------------------------------------------------- */

template<class EntryCls, typename ContextCls = int>
class ListArgumentHandler : public AbstractArgumentHandler {
public:
    ListArgumentHandler(CommandHandler* handler, CommandParameter const& parameter, ContextCls const& ctx)
        : AbstractArgumentHandler(handler, parameter, App::instance().height() * 0.66)
        , m_lines((height() - App::instance().context()->character_height() - 10) / (App::instance().context()->character_height() + 2))
    {
        set_entries(m_current_context);
    }

    void set_entries(ContextCls const& ctx)
    {
        m_current_context = ctx;
        m_entries = std::move(get_entries<EntryCls>(m_parameter, ctx));
        filter_matches();
    }

    [[nodiscard]] CommandParameter const& parameter() const
    {
        return m_parameter;
    }

    void render() override
    {
        box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
        rectangle(SDL_Rect { 2, 2, width() - 4, height() - 4 }, { 0xff, 0xff, 0xff, 0xff });
        render_fixed(8, 8, m_parameter.prompt);
        auto y = App::instance().context()->character_height() + 10;
        auto count = 0;
        for (auto const& match : m_matches) {
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
                    App::instance().context()->character_height() + 1
                };
                box(r, App::instance().color(PaletteIndex::CurrentLineFill));
                rectangle(r, App::instance().color(PaletteIndex::CurrentLineEdge));
            }
            render_fixed(10, y, match.text);
            y += App::instance().context()->character_height() + 2;
            if (y > height() - App::instance().context()->character_height() - 2)
                break;
            count++;
        }
    }

    bool dispatch(SDL_Keysym sym) override
    {
        if (handle(this, m_matches[m_current].entry, m_current_context, sym))
            return true;
        switch (sym.sym) {
        case SDLK_ESCAPE: {
            m_handler->abort();
            return true;
        }
        case SDLK_RETURN: {
            submit(m_handler, m_matches[m_current].entry, m_current_context);
            return true;
        };
        case SDLK_UP: {
            if (m_current > 0)
                m_current--;
            while (m_current < m_top)
                m_top--;
            return true;
        };
        case SDLK_DOWN: {
            if (m_current < m_matches.size() - 1)
                m_current++;
            while (m_current > m_top + m_lines - 1)
                m_top++;
            return true;
        };
        case SDLK_PAGEUP: {
            if (m_current >= m_lines) {
                m_current -= m_lines;
                if (m_top >= m_lines)
                    m_top -= m_lines;
                else
                    m_top = 0;
            }
            return true;
        };
        case SDLK_PAGEDOWN: {
            if (m_current < m_matches.size() - m_lines) {
                m_current += m_lines;
                m_top += m_lines;
            }
            return true;
        };
        case SDLK_BACKSPACE: {
            if (!m_search_str.empty()) {
                m_search_str.erase(m_search_str.length() - 1, 1);
                filter_matches();
            }
            return true;
        }
        default:
            return false;
        }
    }

    void filter_matches()
    {
        m_matches.clear();
        if (m_search_str.empty()) {
            for (auto const& e : m_entries) {
                m_matches.emplace_back(e, m_current_context);
            }
            return;
        }
        for (auto const& e : m_entries) {
            auto s_ix = 0;
            auto text = line_text(e, m_current_context);
            for (auto ix = 0; ix < text.length(); ++ix) {
                if (toupper(m_search_str[s_ix]) == toupper(text[ix])) {
                    if (++s_ix == m_search_str.length()) {
                        break;
                    }
                }
            }
            if (s_ix == m_search_str.length()) {
                m_matches.emplace_back(e, m_current_context);
            }
        }
    }

    void handle_text_input() override
    {
        m_search_str += App::instance().input_buffer();
        filter_matches();
    }

private:
    struct Match {
        EntryCls entry;
        std::string text;

        explicit Match(EntryCls e, ContextCls const& ctx)
            : entry(std::move(e))
            , text(line_text(entry, ctx))
        {
        }
    };

    std::vector<EntryCls> m_entries;
    ContextCls m_current_context;
    int m_lines;
    int m_top { 0 };
    int m_current { 0 };
    std::vector<Match> m_matches;
    std::string m_search_str;
};

AbstractArgumentHandler::AbstractArgumentHandler(CommandHandler* handler, CommandParameter const& parameter, int height)
    : ModalWidget(App::instance().width() * 0.66, height)
    , m_handler(handler)
    , m_parameter(parameter)
{
}

DefaultArgumentHandler::DefaultArgumentHandler(CommandHandler* handler, CommandParameter const& parameter)
    : AbstractArgumentHandler(handler, parameter, 2 * (App::instance().context()->character_height() + 4) + 12)
{
}

void DefaultArgumentHandler::render()
{
    box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
    rectangle(SDL_Rect { 2, 2, width() - 4, height() - 4 }, { 0xff, 0xff, 0xff, 0xff });
    render_fixed(8, 4, m_parameter.prompt);
    render_fixed(8, App::instance().context()->character_height() + 12, m_value);

    static auto time_start = std::chrono::system_clock::now();
    auto time_end = std::chrono::system_clock::now();
    const long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
    if (elapsed > 400) {
        SDL_Rect r {
            8 + App::instance().context()->character_width() * m_pos,
            App::instance().context()->character_height() + 12,
            1,
            App::instance().context()->character_height()
        };
        box(r, App::instance().color(PaletteIndex::Cursor));
        if (elapsed > 800)
            time_start = time_end;
    }
}

bool DefaultArgumentHandler::dispatch(SDL_Keysym sym)
{
    switch (sym.sym) {
    case SDLK_ESCAPE: {
        m_handler->abort();
        return true;
    }
    case SDLK_RETURN: {
        m_handler->argument_done(m_value);
        return true;
    };
    case SDLK_LEFT: {
        if (m_pos > 0)
            m_pos--;
        return true;
    }
    case SDLK_RIGHT: {
        if (m_pos < m_value.length())
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

CommandHandler::CommandHandler(Command const& command)
    : ModalWidget(App::instance().width() / 2, App::instance().height() / 2)
    , m_command(command)
{
}

void CommandHandler::render()
{
    if (m_current_parameter < 0) {
        dismiss();
        return;
    }
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
            m_current_handler = new ListArgumentHandler<fs::directory_entry, fs::path>(this, parameter, fs::current_path());
            break;
        case CommandParameterType::Command: {
            m_current_handler = new ListArgumentHandler<Command>(this, parameter, 0);
        } break;
        case CommandParameterType::Buffer: {
            m_current_handler = new ListArgumentHandler<fs::path>(this, parameter, 0);
        } break;
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
    if (m_current_handler != nullptr)
        m_current_handler->dismiss();
    m_current_handler = nullptr;
    m_current_parameter++;
}

void CommandHandler::abort()
{
    if (m_current_handler != nullptr)
        m_current_handler->dismiss();
    m_current_handler = nullptr;
    m_current_parameter = -1;
}

} // Scratch
