/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <MessageBar.h>

namespace Scratch {

MessageBar::MessageBar()
    : WindowedWidget(App::instance()->rows() - 1, 0, 1, App::instance()->columns())
{
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
    App::instance()->log("MessageBar::render");
    if (m_message.empty())
        return;
    mvwaddstr(window(), 0, 0, m_message.c_str());
    wclrtobot(window());
    wrefresh(window());
    m_message = "";
}

}
