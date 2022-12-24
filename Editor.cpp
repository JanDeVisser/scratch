/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <Editor.h>

namespace Scratch {

Editor::Editor()
    : WindowedWidget(1, 0, App::instance()->rows() - 3, App::instance()->columns())
    , m_document(std::make_shared<Document>())
{
    keypad(window(), true);
}

void Editor::render()
{
    wclear(window());
    for (size_t screen_line = 0; (screen_line < App::instance()->rows() - 3) && (screen_top() + screen_line < document()->line_count()); ++screen_line) {
        mvwaddstr(window(), screen_line, 0, document()->line(screen_top() + screen_line).c_str());
    }
    wmove(window(), point_line() - screen_top(), virtual_point_column() - screen_left());
    wrefresh(window());
}

void Editor::post_render()
{
    wmove(window(), point_line() - screen_top(), virtual_point_column() - screen_left());
    wrefresh(window());
}

bool Editor::handle(int key)
{
    switch (key) {
    case KEY_UP:
        if (m_point_line > 0) {
            m_point_line--;
            m_virtual_point_column = m_point_column;
            if (m_point_column > m_document->line_length(m_point_line))
                m_virtual_point_column = m_document->line_length(m_point_line);
            if (m_screen_top > m_point_line)
                m_screen_top = m_point_line;
        }
        break;
    case KEY_DOWN:
        if (m_point_line < (m_document->line_count() - 1)) {
            m_point_line++;
            m_virtual_point_column = m_point_column;
            if (m_point_column > m_document->line_length(m_point_line))
                m_virtual_point_column = m_document->line_length(m_point_line);
            if (m_point_line - m_screen_top >= App::instance()->rows() - 3)
                m_screen_top = m_point_line - App::instance()->rows() + 4;
        }
        break;
    case KEY_LEFT:
        if (m_point_column > 0) {
            m_point_column = m_virtual_point_column - 1;
            m_virtual_point_column = m_point_column;
        }
        break;
    case KEY_RIGHT:
        if ((m_point_column == m_virtual_point_column) && m_virtual_point_column < m_document->line_length(m_point_line)) {
            m_point_column++;
            m_virtual_point_column = m_point_column;
        }
        break;
    case KEY_BACKSPACE:
        m_document->backspace(m_point_column, m_point_line);
        break;
    case 10:
        m_document->split_line(m_point_column, m_point_line);
        break;
    default:
        m_document->insert(m_point_column, m_point_line, static_cast<char>(key));
        break;
    }
    return true;
}

std::string Editor::status()
{
    char buffer[81];
    snprintf(buffer, 80, "%-20.20s %4d : %4d", document()->filename().c_str(), point_line(), point_column());
    return std::string(buffer);
}

}
