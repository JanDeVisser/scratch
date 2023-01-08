/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <SDLContext.h>
#include <Widget.h>

namespace Scratch {

Widget::Widget()
{
}

void Widget::render()
{
}

//bool Widget::handle(KeyCode key)
//{
//    return false;
//}

WindowedWidget::WindowedWidget(int left, int top, int width, int height)
    : Widget()
    , m_left(left)
    , m_top(top)
    , m_width(width)
    , m_height(height)
{
    if (m_width <= 0)
        m_width = App::instance().width() - left - m_width;
    if (m_height <= 0)
        m_height = App::instance().height() - top - m_height;
}

WindowedWidget::WindowedWidget(Vector<int,4> const& outline, Vector<int,4> const& margins)
    : Widget()
    , m_left(outline[0])
    , m_top(outline[1])
    , m_width(outline[2])
    , m_height(outline[3])
    , m_left_margin(margins[0])
    , m_right_margin(margins[1])
    , m_top_margin(margins[2])
    , m_bottom_margin(margins[3])
{
    if (m_width <= 0)
        m_width = App::instance().width() - m_left + m_width;
    if (m_height <= 0)
        m_height = App::instance().height() - m_top + m_height;
}

Vector<int,2>  WindowedWidget::position() const
{
    return Vector<int,2> { m_left, m_top };
}

Vector<int,2>  WindowedWidget::size() const
{
    return Vector<int,2>  { m_width, m_height };
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

Vector<int,4> WindowedWidget::outline() const
{
    return Vector<int,4> { left(), top(), width(), height() };
}

Vector<int,4> WindowedWidget::content_rect() const
{
    return Vector<int,4> { left() + m_left_margin, top() + m_top_margin,
        width() - m_left_margin - m_right_margin, height() - m_top_margin - m_bottom_margin };
}

Vector<int,4> WindowedWidget::margins() const
{
    return Vector<int,4> { left_margin(), right_margin(), top_margin(), bottom_margin() };
}

int WindowedWidget::left_margin() const
{
    return m_left_margin;
}

int WindowedWidget::right_margin() const
{
    return m_right_margin;
}

int WindowedWidget::top_margin() const
{
    return m_top_margin;
}

int WindowedWidget::bottom_margin() const
{
    return m_bottom_margin;
}

int WindowedWidget::content_width() const
{
    return width() - left_margin() - right_margin();
}

int WindowedWidget::content_height() const
{
    return height() - top_margin() - bottom_margin();
}

void WindowedWidget::render_text(int x, int y, std::string const& text, SDL_Color const& color)
{
    App::instance().context()->render_text(left() + left_margin() + x, top() + top_margin() + y,
        text, color);
}

}
