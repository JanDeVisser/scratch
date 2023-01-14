/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <SDL.h>

#include <Forward.h>
#include <Geometry.h>

namespace Scratch {

class Widget : public std::enable_shared_from_this<Widget> {
public:
    virtual ~Widget() = default;

    virtual void render();
    virtual bool dispatch(SDL_Keysym) { return false; }
    virtual void handle_text_input() { }
    virtual std::vector<std::string> status() { return {}; }

    //[[nodiscard]] virtual bool handle(KeyCode);

    static int char_height;
    static int char_width;
protected:
    Widget() = default;

private:
};

class WindowedWidget : public Widget {
public:
    WindowedWidget(int, int, int, int);
    WindowedWidget(Vector<int,4> const&, Vector<int,4> const&);
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
    void set_render(std::function<void()>);
    void set_dispatch(std::function<bool(SDL_Keysym)>);
    void set_text_input(std::function<void()>);
    void render() override;
    bool dispatch(SDL_Keysym) override;
    void handle_text_input() override;

    SDL_Rect render_fixed(int, int, std::string const&, SDL_Color const& = SDL_Color { 255, 255, 255, 255 }) const;
    SDL_Rect render_fixed_right_aligned(int, int, std::string const&, SDL_Color const& = SDL_Color { 255, 255, 255, 255 }) const;

    SDL_Rect normalize(SDL_Rect const&);
    void box(SDL_Rect const&, SDL_Color);
    void rectangle(SDL_Rect const&, SDL_Color);

private:
    int m_left;
    int m_top;
    int m_width;
    int m_height;

    int m_left_margin { 0 };
    int m_top_margin { 0 };
    int m_right_margin { 0 };
    int m_bottom_margin { 0 };

    std::function<void()> m_render { nullptr };
    std::function<bool(SDL_Keysym)> m_dispatch { nullptr };
    std::function<void()> m_text_input { nullptr };
};

class ModalWidget : public WindowedWidget {
public:
    ModalWidget(int, int, int, int);
    ModalWidget(Vector<int,4> const&, Vector<int,4> const&);
    void dismiss();
private:
};

}
