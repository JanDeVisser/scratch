/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Forward.h>
#include <Geometry.h>

namespace Scratch {

class Widget : public std::enable_shared_from_this<Widget> {
public:
    virtual ~Widget() = default;

    virtual void render();

    //[[nodiscard]] virtual bool handle(KeyCode);

protected:
    Widget();

private:
};

class WindowedWidget : public Widget {
public:
    [[nodiscard]] int top() const;
    [[nodiscard]] int left() const;
    [[nodiscard]] int height() const;
    [[nodiscard]] int width() const;

    [[nodiscard]] Vector<int,2>  position() const;
    [[nodiscard]] Vector<int,2>  size() const;
    [[nodiscard]] Vector<int,4> outline() const;
    [[nodiscard]] Vector<int,4> content_rect() const;
    [[nodiscard]] Vector<int,4> margins() const;
    [[nodiscard]] int left_margin() const;
    [[nodiscard]] int right_margin() const;
    [[nodiscard]] int top_margin() const;
    [[nodiscard]] int bottom_margin() const;
    [[nodiscard]] int content_width() const;
    [[nodiscard]] int content_height() const;

    void render_text(int, int, std::string const&, SDL_Color const& = SDL_Color { 255, 255, 255, 255 });

protected:
    WindowedWidget(int, int, int, int);
    WindowedWidget(Vector<int,4> const&, Vector<int,4> const&);

private:
    int m_left;
    int m_top;
    int m_width;
    int m_height;

    int m_left_margin { 0 };
    int m_right_margin { 0 };
    int m_top_margin { 0 };
    int m_bottom_margin { 0 };

};

}
