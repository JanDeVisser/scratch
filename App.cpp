/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <Menu.h>
#include <MessageBar.h>
#include <StatusBar.h>

namespace Scratch {

pApp App::s_app { nullptr };

pApp const& App::create(std::string name)
{
    assert(s_app == nullptr);
    s_app = std::shared_ptr<App>(new App(std::move(name)));
    return s_app;
}

pApp const& App::instance()
{
    return s_app;
}

App::App(std::string name)
    : m_name(std::move(name))
{
    if (!getenv("TERM"))
        setenv("TERM", "xterm-256color", 1);
    initscr();
    clear();
    noecho();
    raw();
    nonl();
    curs_set(1);
    intrflush(stdscr, false);
    keypad(stdscr, true);
    getmaxyx(stdscr, m_rows, m_cols);
}

App::~App()
{
}

void App::event_loop()
{
    while (!m_quit) {
        log("loop()");
        render_app();
        auto *w = stdscr;
        if (focus() != nullptr)
            w = focus()->window();
        int key = wgetch(w);
        log("keyname: %d = %s", key, keyname(key));
        dispatch(key);
    }
    clear();
    endwin();
}

void App::render_app()
{
    log("App::render_app");
    auto apply = [this](auto callback) -> void {
        Widgets queue { shared_from_this() };
        do {
            auto current = queue.back();
            queue.pop_back();
            queue.insert(queue.end(),
                     current->components().begin(),
                     current->components().end());
            callback(current);
        } while (!queue.empty());
    };
    apply([](auto& widget) { widget->pre_render(); });
    apply([](auto& widget) { widget->render(); });
    apply([](auto& widget) { widget->post_render(); });
}

void App::dispatch(int key)
{
    Widgets queue { shared_from_this() };
    do {
        auto current = queue.back();
        queue.pop_back();
        queue.insert(queue.end(),
                 std::make_reverse_iterator(current->components().end()),
                 std::make_reverse_iterator(current->components().begin()));
        if (current->handle(key))
            return;
    } while (!queue.empty());
}

bool App::handle(int key)
{
    if (key == ctrl('q')) {
        quit();
        return true;
    }
    return false;
}

WINDOW * App::window() const
{
    return stdscr;
}

void App::vmessage(char const* msg, va_list args)
{
    auto messenger = get_component<Messenger>();
    if (messenger != nullptr) {
        messenger->vmessage(msg, args);
    }
}

void App::vlog(char const* msg, va_list args)
{
    auto logger = m_logger;
    if (logger == nullptr) {
        logger = get_component<Logger>();
    }
    if (logger != nullptr) {
        logger->vlog(msg, args);
    }
}

void App::logger(pLogger l)
{
    m_logger = std::move(l);
}
pWindowedWidget const& App::focus()
{
    return m_focus;
}

void App::focus(pWindowedWidget f)
{
    m_focus = std::move(f);
}

}
