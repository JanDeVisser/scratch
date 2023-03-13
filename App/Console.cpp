/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App/Console.h>
#include <App/Editor.h>
#include <Widget/App.h>
#include <Scribble/Parser.h>
#include <Scribble/Scribble.h>

namespace Scratch {

ConsoleCommands::ConsoleCommands()
{
}

ConsoleCommands Console::s_console_commands;

Console::Console(Editor* editor)
    : Buffer(editor)
{
    m_commands = &s_console_commands;
}

void Console::render()
{
    size_t lines { 0 };
    if (!m_statements.empty())
        lines = m_statements.back().line + m_statements.back().lines.size();
    lines += m_current.lines.size();
    size_t top_line = (lines > editor()->rows()) ? lines - editor()->rows() : 0;

    auto render_statement = [this](Statement const& stmt, size_t first_line) -> void {
        PaletteIndex prompt_color = PaletteIndex::ANSIBrightBlack;
        if (stmt.node == nullptr || !stmt.node->is_complete())
            prompt_color = PaletteIndex::ANSIBrightGreen;
        else if (stmt.result.is_error())
            prompt_color = PaletteIndex::ANSIBrightRed;

        if (stmt.lines.empty()) {
            std::string_view prompt = "*> ";
            editor()->append(DisplayToken { prompt, prompt_color });
            return;
        }
        for (auto ix = first_line; ix < stmt.lines.size(); ++ix) {
            std::string_view prompt = (ix == 0) ? "*> " : "-> ";
            auto l = stmt.lines[ix];
            auto len = 0u;
            editor()->append(DisplayToken { prompt, prompt_color });
            for (auto const& token : l.tokens) {
                auto t = token.value();
                if (len + 3 + t.length() < m_screen_left) {
                    len += t.length();
                    continue;
                } else if (len + 3 < m_screen_left) {
                    t = t.substr(m_screen_left - len);
                } else if (len + 3 + t.length() > m_screen_left + columns()) {
                    t = t.substr(0, m_screen_left + columns() - len);
                }
                editor()->append(token_for(token.code(), t));
                len += token.value().length();
                if (len + 3 >= m_screen_left + editor()->columns())
                    break;
            }
            editor()->newline();
        }
    };

    size_t row = 0;
    size_t next_line = 0;
    for (auto const& stmt : m_statements) {
        next_line += stmt.line + stmt.lines.size();
        if (stmt.line + stmt.lines.size() < top_line)
            continue;
        render_statement(stmt, (stmt.line >= top_line) ? 0 : top_line - stmt.line);
        row += stmt.lines.size();
    }
    render_statement(m_current, (next_line >= top_line) ? 0 : top_line - next_line);
    editor()->text_cursor(static_cast<int>(row + m_cursor_line), static_cast<int>(m_cursor_column + 3 - m_screen_left));
}

bool Console::dispatch(SDL_Keysym sym)
{
    std::string statement;
    if (m_current.text != nullptr)
        statement = m_current.text->str();
    switch (sym.sym) {
    case SDLK_RETURN: {
        if (m_current.node != nullptr && m_current.node->is_complete())
            execute();
        return true;
    };
    case SDLK_LEFT: {
        if (m_cursor_column > 0)
            m_cursor_column--;
        return true;
    }
    case SDLK_RIGHT: {
        if (m_cursor_column < statement.length())
            m_cursor_column++;
        return true;
    }
    case SDLK_HOME:
        m_cursor_column = 0;
        return true;
    case SDLK_END:
        m_cursor_column = statement.length();
        return true;
    case SDLK_BACKSPACE: {
        if (m_cursor_column > 0) {
            statement.erase(m_cursor_column - 1, 1);
            m_cursor_column--;
        }
        return true;
    }
    default:
        return false;
    }
}

void Console::compile(std::string const& statement)
{
    m_current.text = std::make_shared<StringBuffer>(statement);
    m_current.node = nullptr;
    m_current.result = Value {};
    m_current.lines.clear();
    m_current.lines.emplace_back();
    m_current.lines.back().start_index = 0;
    auto length = 0u;
    auto project_maybe = compile_project("**Console**", m_current.text);
    if (!project_maybe.is_error()) {
        m_current.node = std::dynamic_pointer_cast<Project>(project_maybe.value());
        for (auto const& token : m_current.node->modules()[0]->tokens()) {
            if (token.code() == TokenCode::EndOfFile)
                break;
            switch (token.code()) {
            case TokenCode::NewLine:
                length += 1;
                m_current.lines.emplace_back();
                m_current.lines.back().start_index = static_cast<int>(length);
                break;
            default:
                m_current.lines.back().tokens.push_back(token);
                length += token.value().length();
                break;
            }
        }
    } else {
        m_current.result = Value { ErrorCode::SyntaxError };
    }
}

void Console::handle_text_input()
{
    std::string statement;
    if (m_current.text != nullptr)
        statement = m_current.text->str();
    auto str = App::instance().input_buffer();
    if (str.empty())
        return;
    if (m_cursor_column <= statement.length()) {
        statement.insert(m_cursor_column, str);
        m_cursor_column += str.length();
        compile(statement);
    }
}

void Console::mousedown(int line, int column)
{
}

void Console::motion(int line, int column)
{
}

void Console::click(int line, int column, int clicks)
{
}

void Console::wheel(int lines)
{
}

void Console::execute()
{
    assert(m_current.node && m_current.node->is_complete());
    auto result = interpret(m_current.node, m_ctx);
    if (result.is_error()) {
        m_current.result = result.error().to_string();
    } else {
        auto r = std::dynamic_pointer_cast<Interp::ExpressionResult>(result.value());
        m_current.result = r->value();
    }
    m_current.line = 0u;
    if (!m_statements.empty())
        m_current.line = m_statements.back().line + 2;
    m_statements.push_back(m_current);
    m_current = { 0, nullptr, nullptr, Value {}, {} };
    m_cursor_column = 0;
}

} // Scratch
