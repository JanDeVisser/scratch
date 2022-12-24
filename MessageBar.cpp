/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <MessageBar.h>

namespace Scratch {

MessageBar::MessageBar()
    : Widget()
{
    m_window = newwin(1, App::instance()->columns(), App::instance()->rows() - 1, 0);
}

void MessageBar::vmessage(char const* msg, va_list args)
{
    static char buffer[1024];
    vsnprintf(buffer, 1023, msg, args);
    m_message = std::string(buffer);
}

void MessageBar::clear()
{
    m_message = "";
}

void MessageBar::render()
{
    if (m_message.empty())
        return;
    wclear(m_window);
    mvwaddstr(m_window, 0, 0, m_message.c_str());
    wrefresh(m_window);
    m_message = "";
}

}
