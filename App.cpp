/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <array>
#include <cstdio>

#include <App.h>
#include <Editor.h>
#include <Forward.h>
#include <MessageBar.h>
#include <StatusBar.h>

namespace Scratch {

App* App::s_app { nullptr };

App& App::instance()
{
    oassert(s_app != nullptr, "No App instantiated");
    return *s_app;
}

App::App(std::string name, Display *display)
    : m_name(std::move(name))
    , m_display(display)
{
    oassert(s_app == nullptr, "App is a singleton");
    s_app = this;
}

App::~App()
{
}

void App::event_loop()
{
    while (!m_quit) {
        render_app();
        auto key_maybe = Display::getkey();
        if (key_maybe.is_error()) {
            if (key_maybe.error() == EINTR) {
                auto resize_maybe = resize();
                if (resize_maybe.is_error()) {
                    fprintf(stderr, "ERROR: %s\n", resize_maybe.error().c_str());
                    break;
                }
                continue;
            }
            fprintf(stderr, "ERROR: something went wrong during reading of the user input: %s\n", strerror(errno));
            break;
        }
        dispatch(key_maybe.value());
    }
    printf("\033[2J\033[H");
}

void App::render_app()
{
    display()->clear();
    for (auto const& c : components()) {
        c->pre_render();
    }
    for (auto const& c : components()) {
        c->render();
    }
    for (auto iter = components().rbegin(); iter != components().rend(); ++iter) {
        (*iter)->post_render();
    }
    display()->render();
}

void App::dispatch(KeyCode key)
{
    if (!handle(key)) {
        for (auto const& c : components()) {
            if (c->handle(key))
                break;
        }
    }
}

bool App::handle(KeyCode key)
{
    if (key == CTRL_Q) {
        quit();
        return true;
    }
    return false;
}

ErrorOr<void,std::string> App::resize()
{
    return {};
}

void App::vmessage(char const* msg, va_list args)
{
    auto messenger = get_component<Messenger>();
    if (messenger != nullptr) {
        messenger->vmessage(msg, args);
    }
}

void App::add_component(Widget* component)
{
    m_components.emplace_back(component);
}

}
