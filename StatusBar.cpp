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
    if (status.length() > display()->columns() - 2)
        status = status.substr(0, display()->columns() - 2);

    char buffer[display()->columns() + 1];
    memset(buffer, ' ', display()->columns());
    buffer[display()->columns()] = '\0';
    memcpy(buffer + display()->columns()-status.length()-2, status.c_str(), status.length());
    display()->append({ std::string(buffer), DisplayStyle::Reverse });
    display()->newline();
}

}
