/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Forward.h>

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

protected:
    WindowedWidget(int, int, int, int);

private:
    int m_top;
    int m_left;
    int m_height;
    int m_width;
};

}
