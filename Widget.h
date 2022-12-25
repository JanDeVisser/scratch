/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <ncurses.h>

#include <Forward.h>

namespace Scratch {

class Widget : public std::enable_shared_from_this<Widget> {
public:
    virtual ~Widget() = default;

    virtual void pre_render();
    virtual void render();
    virtual void post_render();

    [[nodiscard]] virtual bool handle(int);

    void add_component(pWidget const&);
    [[nodiscard]] pWidget const& parent() const { return m_parent; }
    [[nodiscard]] virtual Widgets const& components() { return m_components; }

    template <class ComponentClass>
//  requires std::derived_from<ComponentClass, Widget>
    std::shared_ptr<ComponentClass> get_component()
    {
        for (auto& c : components()) {
            if (auto casted = std::dynamic_pointer_cast<ComponentClass>(c); casted != nullptr)
                return casted;
            if (auto child = c->get_component<ComponentClass>(); child != nullptr)
                return child;
        }
        return nullptr;
    }

protected:
    Widget();
    [[nodiscard]] virtual WINDOW* window() const;

private:
    pApp m_app;
    pWidget m_parent;
    Widgets m_components;
};

class WindowedWidget : public Widget {
public:
    [[nodiscard]] int top() const;
    [[nodiscard]] int left() const;
    [[nodiscard]] int height() const;
    [[nodiscard]] int width() const;
    [[nodiscard]] WINDOW * window() const override;

protected:
    WindowedWidget(int, int, int, int);

private:
    WINDOW * m_window;
    int m_top;
    int m_left;
    int m_height;
    int m_width;
};

}
