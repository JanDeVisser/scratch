/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <cctype>

#include <App.h>
#include <Editor.h>

namespace Scratch {

extern_logging_category(scratch);

enum class HighlightColor {
    Comment = 1,
    Identifier,
    Keyword,
    Number,
    String,
};

Editor::Editor()
    : WindowedWidget(1, 0, App::instance().rows() - 3, App::instance().columns())
{
}

void Editor::render()
{
    debug(scratch, "Editor::render");

    int screen_line = 0;
    display()->clear();
    document().rewind();
    for (auto token = document().lex(); token.code() != TokenCode::EndOfFile; token = document().lex()) {
        switch (token.code()) {
        case TokenCode::NewLine:
            display()->newline();
            break;
        case TokenCode::Comment:
            display()->append({ token.value(), DisplayStyle::Colored, { ForegroundColor::Gray, BackgroundColor::Black } });
            break;
        case TokenCode::Identifier:
            display()->append({ token.value() });
            break;
        case KeywordConst:
        case KeywordIf:
        case KeywordElse:
        case KeywordNamespace:
        case KeywordNullptr:
        case KeywordWhile:
            display()->append({ token.value(), DisplayStyle::Colored, { ForegroundColor::Red, BackgroundColor::Black } });
            break;
        default:
            display()->append({ token.value() });
            break;
        }
    }
    display()->render(m_point_line, m_point_column);
}

void Editor::post_render()
{
}

bool Editor::handle(KeyCode key)
{
    switch (key) {
    case KEY_UP:
        if (m_point_line > 0) {
            m_point_line--;
            if (m_point_column > document().line_length(m_point_line))
                m_point_column = document().line_length(m_point_line);
            if (m_screen_top > m_point_line)
                m_screen_top = m_point_line;
        }
        break;
    case KEY_DOWN:
        if (m_point_line < (document().line_count() - 1)) {
            m_point_line++;
            if (m_point_column > document().line_length(m_point_line))
                m_point_column = document().line_length(m_point_line);
            if (m_point_line - m_screen_top >= App::instance().rows())
                m_screen_top = m_point_line - App::instance().rows() + 1;
        }
        break;
    case KEY_LEFT:
        if (m_point_column > 0) {
            --m_point_column;
        }
        break;
    case KEY_RIGHT:
        if (m_point_column < document().line_length(m_point_line)) {
            ++m_point_column;
        }
        break;
    case KEY_BACKSPACE:
    case 127:
    case '\b':
        document().backspace(m_point_column, m_point_line);
        if (m_point_column > 0) {
            m_point_column--;
        } else if (m_point_line > 0) {
            m_point_line--;
            m_point_column = document().line_length(m_point_line);
        }
        break;
    case 13:
        document().split_line(m_point_column, m_point_line);
        ++m_point_line;
        m_point_column = 0;
        break;
    default:
        if (isprint(key)) {
            document().insert(m_point_column, m_point_line, static_cast<char>(key));
            m_point_column++;
        } else {
            return false;
        }
        break;
    }
    return true;
}

std::string Editor::status()
{
    char buffer[81];
    snprintf(buffer, 80, "%-20.20s %4d : %4d", document().filename().c_str(), point_line(), point_column());
    return std::string(buffer);
}

}
