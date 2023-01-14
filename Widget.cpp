/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <SDL2_gfxPrimitives.h>

#include <App.h>
#include <SDLContext.h>
#include <Widget.h>

namespace Scratch {

int Widget::char_height { 0 };
int Widget::char_width { 0 };

void Widget::render()
{
}

WindowedWidget::WindowedWidget(int left, int top, int width, int height)
    : Widget()
    , m_left(left)
    , m_top(top)
    , m_width(width)
    , m_height(height)
{
    if (m_left < 0)
        m_left = App::instance().width() + m_left;
    if (m_top < 0)
        m_top = App::instance().height() + m_top;
    if (m_width <= 0)
        m_width = App::instance().width() - left - m_width;
    if (m_height <= 0)
        m_height = App::instance().height() - top - m_height;
}

WindowedWidget::WindowedWidget(Vector<int, 4> const& outline, Vector<int, 4> const& margins)
    : Widget()
    , m_left(outline[0])
    , m_top(outline[1])
    , m_width(outline[2])
    , m_height(outline[3])
    , m_left_margin(margins[0])
    , m_top_margin(margins[1])
    , m_right_margin(margins[2])
    , m_bottom_margin(margins[3])
{
    if (m_left < 0)
        m_left = App::instance().width() + m_left;
    if (m_top < 0)
        m_top = App::instance().height() + m_top;
    if (m_width <= 0)
        m_width = App::instance().width() - m_left + m_width;
    if (m_height <= 0)
        m_height = App::instance().height() - m_top + m_height;
}

Vector<int, 2> WindowedWidget::position() const
{
    return Vector<int, 2> { m_left, m_top };
}

Vector<int, 2> WindowedWidget::size() const
{
    return Vector<int, 2> { m_width, m_height };
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

Vector<int, 4> WindowedWidget::outline() const
{
    return Vector<int, 4> { left(), top(), width(), height() };
}

Vector<int, 4> WindowedWidget::content_rect() const
{
    return Vector<int, 4> { left() + m_left_margin, top() + m_top_margin,
        width() - m_left_margin - m_right_margin, height() - m_top_margin - m_bottom_margin };
}

Vector<int, 4> WindowedWidget::margins() const
{
    return Vector<int, 4> { left_margin(), right_margin(), top_margin(), bottom_margin() };
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

void WindowedWidget::set_render(std::function<void()> r)
{
    m_render = std::move(r);
}

void WindowedWidget::set_dispatch(std::function<bool(SDL_Keysym)> d)
{
    m_dispatch = std::move(d);
}

void WindowedWidget::set_text_input(std::function<void()> t)
{
    m_text_input = std::move(t);
}

void WindowedWidget::render()
{
    if (m_render != nullptr)
        m_render();
}

bool WindowedWidget::dispatch(SDL_Keysym sym)
{
    if (m_dispatch != nullptr)
        return m_dispatch(sym);
    return false;
}

void WindowedWidget::handle_text_input()
{
    if (m_text_input != nullptr)
        m_text_input();
}

SDL_Rect WindowedWidget::render_fixed(int x, int y, std::string const& text, SDL_Color const& color) const
{
    auto ret = App::instance().context()->render_fixed(left() + left_margin() + x, top() + top_margin() + y,
        text, color);
    ret.x -= left() - left_margin();
    ret.y -= top() - top_margin();
    return ret;
}

SDL_Rect WindowedWidget::render_fixed_right_aligned(int x, int y, std::string const& text, SDL_Color const& color) const
{
    auto ret = App::instance().context()->render_fixed_right_aligned(
        left() + left_margin() + x, top() + top_margin() + y,
        text, color);
    ret.x -= left() - left_margin();
    ret.y -= top() - top_margin();
    return ret;
}

SDL_Rect WindowedWidget::normalize(SDL_Rect const& rect)
{
    SDL_Rect r = rect;
    if (r.x < 0)
        r.x = content_width() + r.x;
    if (r.y < 0)
        r.y = content_height() + r.y;
    if (r.w <= 0)
        r.w = content_width() - r.x + r.w;
    if (r.h <= 0)
        r.h = content_height() - r.y + r.h;
    r.x = clamp(r.x, 0, content_width());
    r.y = clamp(r.y, 0, content_height());
    r.w = clamp(r.w, 0, content_width() - r.x);
    r.h = clamp(r.h, 0, content_height() - r.y);
    return r;
}

void WindowedWidget::box(SDL_Rect const& rect, SDL_Color color)
{
    auto r = normalize(rect);
    boxColor(
        App::instance().renderer(),
        left() + left_margin() + r.x,
        top() + top_margin() + r.y,
        left() + left_margin() + r.x + r.w,
        top() + top_margin() + r.y + r.h,
        *((uint32_t*)&color));
}

void WindowedWidget::rectangle(SDL_Rect const& rect, SDL_Color color)
{
    auto r = normalize(rect);
    rectangleColor(
        App::instance().renderer(),
        left() + left_margin() + r.x,
        top() + top_margin() + r.y,
        left() + left_margin() + r.x + r.w,
        top() + top_margin() + r.y + r.h,
        *((uint32_t*)&color));
}

ModalWidget::ModalWidget(int x, int y, int w, int h)
    : WindowedWidget(x, y, w, h)
{
}

ModalWidget::ModalWidget(Vector<int, 4> const& outline, Vector<int, 4> const& margins)
    : WindowedWidget(outline, margins)
{

}

void ModalWidget::dismiss()
{
    if (App::instance().modal() == this)
        App::instance().dismiss_modal();
}

}
