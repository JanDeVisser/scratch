/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <Widget.h>

namespace Scratch {

Widget::Widget()
    : m_app(App::instance())
{
}

void Widget::add_component(pWidget const& component)
{
    m_components.push_back(component);
    component->m_parent = shared_from_this();
}

void Widget::pre_render()
{
}

void Widget::render()
{
}

void Widget::post_render()
{
}

bool Widget::handle(int key)
{
    return false;
}

WINDOW * Widget::window() const
{
    return parent()->window();
}

WindowedWidget::WindowedWidget(int top, int left, int height, int width)
    : Widget()
    , m_window(newwin(height, width, top, left))
    , m_top(top)
    , m_left(left)
    , m_height(height)
    , m_width(width)
{
}

int WindowedWidget::top() const
{
    return m_top;
}

int WindowedWidget::left() const
{
    return m_left;
}

int WindowedWidget::height() const
{
    return m_height;
}

int WindowedWidget::width() const
{
    return m_width;
}

WINDOW * WindowedWidget::window() const
{
    return m_window;
}

}
