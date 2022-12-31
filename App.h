/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include <core/Logging.h>

#include <Display.h>
#include <Widget.h>

namespace Scratch {

extern_logging_category(scratch);

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

class App : public Widget, public Messenger {
public:
    App(std::string, Display *);
    static App& instance();

    ~App() override;
    void quit() { m_quit = true; }
    [[nodiscard]] bool is_running() const { return !m_quit; }
    [[nodiscard]] Display* display() { return m_display.get(); }

    [[nodiscard]] int rows() const { return m_display->rows(); }
    [[nodiscard]] int columns() const { return m_display->columns(); }
    [[nodiscard]] bool handle(KeyCode) override;
    void vmessage(char const*, va_list) override;

    void event_loop();

    void add_component(Widget*);
    [[nodiscard]] virtual std::vector<std::unique_ptr<Widget>> const& components() { return m_components; }

    template <class ComponentClass>
//  requires std::derived_from<ComponentClass, Widget>
    ComponentClass* get_component()
    {
        for (auto& c : components()) {
            if (auto casted = dynamic_cast<ComponentClass*>(c.get()); casted != nullptr)
                return casted;
        }
        return nullptr;
    }

private:
    static App* s_app;

    void dispatch(KeyCode);
    void render_app();
    ErrorOr<void,std::string> resize();

    std::string m_name;
    std::unique_ptr<Display> m_display;
    bool m_quit { false };
    std::vector<std::unique_ptr<Widget>> m_components;
};

}
