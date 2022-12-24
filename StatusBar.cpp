/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <StatusBar.h>

namespace Scratch {

StatusBar::StatusBar()
    : WindowedWidget(App::instance()->rows() - 2, 0, 1, App::instance()->columns())
{
}

void StatusBar::render()
{
    auto reporter = App::instance()->get_component<StatusReporter>();
    std::string status;
    if (reporter != nullptr)
        status = reporter->status();

    wclear(window());
    wattron(window(), A_REVERSE);
    wprintw(window(), "%-*.*s%s  ", App::instance()->columns() - status.length() - 2, App::instance()->columns() - status.length() - 2, " ", status.c_str());
    wattroff(window(), A_REVERSE);
    wrefresh(window());
}

}
