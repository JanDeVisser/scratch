/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Widget/App.h"
#include <Scrollbar.h>

namespace Scratch {

ScrollBar::ScrollBar(Widget* scrolls, ScrollDirection direction)
    : m_scrolls(scrolls)
    , m_direction(direction)
{
}

void ScrollBar::release_mouse_down(App* app)
{
    m_mouse_down = false;
    App::instance().active(0);
}

void ScrollBar::check_mouse_down(App* app, Vec2 const& pos, float offset)
{
    if (m_mouse_down)
        return;
    if (App::instance().active())
        return;
    if (intersects(pos.x, pos.y, m_point0.x, m_point0.y, m_point1.x, m_point1.y)) {
        m_mouse_down_offset = offset;
        m_mouse_down_pos = Vec2(pos.x, pos.y);
        m_mouse_down = true;
        App::instance().active((intptr_t)this);
    }
}

void ScrollBar::render()
{
}

}
