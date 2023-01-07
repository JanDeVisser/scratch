/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Geometry.h>
#include <Widget.h>

namespace Scratch {

enum class ScrollDirection {
    Horizontal,
    Vertical,
};


class ScrollBar : public Widget {
public:
    ScrollBar(Widget *scrolls, ScrollDirection);
    [[nodiscard]] Vec2 const& point0() const { return m_point0; }
    void point0(Vec2 const& val) { m_point0 = val; }
    [[nodiscard]] Vec2 const& point1() const { return m_point1; }
    void point1(Vec2 const& val) { m_point1 = val; }

    [[nodiscard]] Vec2 const& mouse_down_pos() const { return m_mouse_down_pos; }
    [[nodiscard]] float mouse_down_offset() const { return m_mouse_down_offset; }
    [[nodiscard]] bool is_mouse_down() const { return m_mouse_down; }
    void release_mouse_down(class App*);
    void check_mouse_down(class App*, Vec2 const&, float);

    void render() override;

private:
    Widget *m_scrolls;
    ScrollDirection m_direction;
    Vec2 m_point0;
    Vec2 m_point1;

    bool m_mouse_down = false;
    float m_mouse_down_offset = 0.0f;
    Vec2 m_mouse_down_pos;
};

}
