/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <App/Buffer.h>
#include <Commands/Command.h>
#include <Scribble/Interp/Interpreter.h>

namespace Scratch {

struct ConsoleCommands : public Commands {
    ConsoleCommands();
};

class Console : public Buffer {
public:
    explicit Console(Editor *);

    std::string title() const override { return "** Console **"; }
    [[nodiscard]] virtual std::string status() const override { return ""; }

    void execute();

    void render() override;
    bool dispatch(SDL_Keysym) override;
    void mousedown(int, int) override;
    void motion(int, int) override;
    void click(int, int, int) override;
    void wheel(int) override;
    void handle_text_input() override;

private:
    struct Statement {
        size_t line { 0 };
        std::shared_ptr<StringBuffer> text { nullptr };
        std::shared_ptr<Project> node { nullptr };
        Value result {};
        std::vector<Line> lines {};

    };

    void compile(std::string const&);

    size_t m_screen_left { 0};
    std::vector<Statement> m_statements;
    Statement m_current;
    size_t m_cursor_line { 0 };
    size_t m_cursor_column { 0 };
    InterpreterContext m_ctx;

    static ConsoleCommands s_console_commands;
};

} // Scratch
