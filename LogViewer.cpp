/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <LogViewer.h>

namespace Scratch {

LogViewer::LogViewer()
    : WindowedWidget(1, 0, App::instance()->rows() - 3, App::instance()->columns())
{
}

void LogViewer::vlog(char const* msg, va_list args)
{
    static char buffer[1024];
    vsnprintf(buffer, 1023, msg, args);
    m_messages.emplace_back(buffer);
}

void LogViewer::render()
{
    wclear(window());
    int offset = 0;
    if (m_messages.size() > App::instance()->rows() - 3)
        offset = m_messages.size() - App::instance()->rows() - 3;
    for (size_t screen_line = 0; (screen_line < App::instance()->rows() - 3) && offset + screen_line < m_messages.size(); ++screen_line) {
        mvwaddstr(window(), screen_line, 0, m_messages[offset+screen_line].c_str());
    }
    wrefresh(window());
}

bool LogViewer::handle(int)
{
    return false;
}

}
