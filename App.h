/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <curses.h>
#include <string>

#include <Widget.h>

namespace Scratch {

class Messenger {
public:
    void message(char const* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        vmessage(msg, args);
        va_end(args);
    }

    virtual void vmessage(char const*, va_list) = 0;
};

class Logger {
public:
    void log(char const* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        vlog(msg, args);
        va_end(args);
    }

    virtual void vlog(char const*, va_list) = 0;
};

class App : public Widget, public Messenger, public Logger {
public:
    static pApp const& create(std::string name);
    static pApp const& instance();

    ~App() override;
    void quit() { m_quit = true; }
    [[nodiscard]] bool is_running() const { return !m_quit; }

    [[nodiscard]] int rows() const { return m_rows; }
    [[nodiscard]] int columns() const { return m_cols; }
    [[nodiscard]] bool handle(int) override;
    void vmessage(char const*, va_list) override;
    void vlog(char const*, va_list) override;
    void logger(pLogger l);
    pWindowedWidget const& focus();
    void focus(pWindowedWidget);

    void event_loop();

protected:
    [[nodiscard]] WINDOW * window() const override;

private:
    static pApp s_app;
    explicit App(std::string);

    void dispatch(int);
    void render_app();
    std::string m_name;
    bool m_quit { false };
    pLogger m_logger { nullptr };
    pWindowedWidget m_focus { nullptr };

    int m_cols;
    int m_rows;
};

}
