/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Format.h>
#include <core/StringUtil.h>

#include <Commands/Command.h>
#include <SDLContext.h>
#include <Scratch.h>

using namespace Obelix;

namespace Scratch {

std::map<std::string, Command> Command::s_commands = {
    {
        "copy-to-clipboard",
        {
            "copy-to-clipboard", "Copy selection to clipboard", {},
            [](strings const&) -> void {
                auto doc = Scratch::editor()->document();
                doc->copy_to_clipboard();
            }
        }
    },
    {
        "cut-to-clipboard",
        {
            "cut-to-clipboard", "Cut selection to clipboard", {},
            [](strings const&) -> void {
                auto doc = Scratch::editor()->document();
                doc->cut_to_clipboard();
            }
        }
    },
    {
        "duplicate-line",
        {
            "duplicate-line", "Duplicate current line", {},
            [](strings const&) -> void {
                Scratch::editor()->document()->duplicate_line();
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
    { "find-first",
        { "find-first", "Find",
            { {
                "Find",
                CommandParameterType::String,
                []() -> std::string {
                    return Scratch::editor()->document()->selected_text();
                }
            } },
            [](strings const& args) -> void {
                Scratch::editor()->document()->find(args[0]);
            } } },

    { "find-next",
        { "find-next", "Find next",
            {},
            [](strings const& args) -> void {
                Scratch::editor()->document()->find_next();
            } } },

    { "goto-line-column",
        { "goto-line-column", "Goto line:column",
            { { "Line:Column to go to", CommandParameterType::String } },
            [](strings const& args) -> void {
                auto line_col = split(args[0], ':');
                if (line_col.size() < 1)
                    return;
                if (auto line_maybe = try_to_long(line_col[0]); line_maybe.has_value()) {
                    int col = -1;
                    if (line_col.size() > 1) {
                        if (auto col_maybe = try_to_long(line_col[1]); col_maybe.has_value())
                            col = col_maybe.value();
                    }
                    Scratch::editor()->move_to(line_maybe.value() - 1, col - 1, false);
                }
            } } },
    { "paste-from-clipboard",
        { "paste-from-clipboard", "Paste text from clipboard", {},
            [](strings const&) -> void {
                auto doc = Scratch::editor()->document();
                doc->paste_from_clipboard();
            } } },
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
        "redo",
        {
            "redo", "Redo edit", { },
            [](strings const&) -> void {
                Scratch::editor()->document()->redo();
            }
        }
    },
    { "reset-font",
        { "reset-font", "Reset editor font", {},
            [](strings const&) -> void {
                auto& app = App::instance();
                app.reset_font();
            } } },
    { "save-current-as",
        { "save-current-as", "Save current file as",
            { { "New file name", CommandParameterType::String } },
            [](strings const& args) -> void { Scratch::editor()->save_file_as(args[0]); } } },
    { "save-all-files",
        { "save-all-files", "Save call files",
            {},
            [](strings const&) -> void { Scratch::editor()->save_all(); } } },
    {
        "save-file",
        {
            "save-file", "Save current file", {},
            [](strings const&) -> void {
                auto editor = Scratch::editor();
                assert(editor != nullptr);
                if (editor->document()->path().empty()) {
                    App::instance().schedule(Command::get("save-current-as"));
                } else {
                    editor->save_file();
                }
            }
        }
    },
    {
        "select-all",
        {
            "select-all", "Select all", {},
            [](strings const&) -> void {
                Scratch::editor()->document()->select_all();
            }
        }
    },
    {
        "select-line",
        {
            "select-line", "Select line", {},
            [](strings const&) -> void {
                Scratch::editor()->document()->select_line();
            }
        }
    },
    {
        "select-word",
        {
            "select-word", "Select word", {},
            [](strings const&) -> void {
                Scratch::editor()->document()->select_word();
            }
        }
    },
    { "shrink-font",
        { "shrink-font", "Shrink editor font", {},
            [](strings const&) -> void {
                auto& app = App::instance();
                app.shrink_font();
            } } },
    {
        "switch-buffer",
        {
            "switch-buffer", "Switch buffer",
            {
                { "Buffer", CommandParameterType::Buffer }
            },
            [](strings const& args) -> void {
                Scratch::editor()->switch_to(args[0]);
            }
        }
    },
    {
        "transpose-lines-down",
        {
            "transpose-lines-down", "Transpose lines down", {},
            [](strings const&) -> void {
                Scratch::editor()->document()->transpose_lines(Document::TransposeDirection::Down);
            }
        }
    },
    {
        "transpose-lines-up",
        {
            "transpose-lines-up", "Transpose lines up", {},
            [](strings const&) -> void {
                Scratch::editor()->document()->transpose_lines(Document::TransposeDirection::Up);
            }
        }
    },
    {
        "undo",
        {
            "undo", "Undo edit", { },
            [](strings const&) -> void {
                Scratch::editor()->document()->undo();
            }
        }
    },
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
    { { SDLK_DOWN, KMOD_ALT }, "transpose-lines-down" },
    { { SDLK_UP, KMOD_ALT }, "transpose-lines-up" },
    { { SDLK_0, KMOD_GUI }, "reset-font" },
    { { SDLK_a, KMOD_CTRL }, "select-all" },
    { { SDLK_b, KMOD_CTRL }, "switch-buffer" },
    { { SDLK_c, KMOD_CTRL }, "copy-to-clipboard" },
    { { SDLK_d, KMOD_CTRL }, "duplicate-line" },
    { { SDLK_f, KMOD_CTRL }, "find-first" },
    { { SDLK_g, KMOD_CTRL }, "goto-line-column" },
    { { SDLK_f, KMOD_CTRL | KMOD_SHIFT }, "find-next" },
    { { SDLK_l, KMOD_CTRL }, "save-all-files" },
    { { SDLK_n, KMOD_CTRL }, "new-buffer" },
    { { SDLK_o, KMOD_CTRL }, "open-file" },
    { { SDLK_q, KMOD_CTRL }, "scratch-quit" },
    { { SDLK_s, KMOD_CTRL }, "save-file" },
    { { SDLK_s, KMOD_CTRL | KMOD_GUI }, "save-current-as" },
    { { SDLK_v, KMOD_CTRL }, "paste-from-clipboard" },
    { { SDLK_x, KMOD_CTRL }, "cut-to-clipboard" },
    { { SDLK_x, KMOD_GUI }, "invoke" },
    { { SDLK_y, KMOD_CTRL }, "redo" },
    { { SDLK_z, KMOD_CTRL }, "undo" },
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

} // Scratch
