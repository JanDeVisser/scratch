/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <StatusBar.h>
#include <Editor.h>

namespace Scratch {

StatusBar::StatusBar()
    : WindowedWidget(App::instance().rows() - 2, 0, 1, App::instance().columns())
{
}

void StatusBar::render()
{
    debug(scratch, "StatusBar::render");
    auto editor = App::instance().get_component<Editor>();
    std::string status;
    if (editor != nullptr)
        status = editor->status();

    char buffer[display()->columns() + 1];
    snprintf(buffer, display()->columns(), "%-*.*s%s  ", (int)App::instance().columns() - status.length() - 2, (int)App::instance().columns() - status.length() - 2, " ", status.c_str());
    display()->append({ std::string(buffer), DisplayStyle::Reverse });
    display()->newline();
}

}
