/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <Editor.h>
#include <LogViewer.h>
#include <Menu.h>
#include <MessageBar.h>
#include <StatusBar.h>
#include <Widget.h>
#include <WidgetStack.h>

namespace Scratch {

void run_app(int argc, char **argv)
{
    auto app = App::create("Scratch");

    auto menu = std::make_shared<MenuBar>(MenuDescriptions {
        { "File", { { "Quit", [&app]() { app->quit(); } } } }
    });
    app->add_component(menu);
    auto stack = std::make_shared<WidgetStack>();
    auto editor = std::make_shared<Editor>();
    editor->document()->load("App.cpp");
    stack->add(editor);
    auto logger = std::make_shared<LogViewer>();
    stack->add(logger);
    app->logger(logger);
    app->add_component(stack);
    app->log("Hello");
    app->add_component(std::make_shared<StatusBar>());
    app->add_component(std::make_shared<MessageBar>());
    app->focus(editor);
    app->event_loop();
}

}

int main(int argc, char** argv)
{
    Scratch::run_app(argc, argv);
    return 0;
}
