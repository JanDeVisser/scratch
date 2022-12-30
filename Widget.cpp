/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <Widget.h>

namespace Scratch {

Widget::Widget()
{
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

bool Widget::handle(KeyCode key)
{
    return false;
}

Display* Widget::display()
{
    return App::instance().display();
}

WindowedWidget::WindowedWidget(int top, int left, int height, int width)
    : Widget()
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

}
