/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <functional>

#include <SDL.h>

#include "SDLContext.h"
#include <App/Forward.h>
#include <Commands/Command.h>
#include <Widget/Geometry.h>

namespace Scratch {

enum class TextAlignment {
    Left,
    Right,
    Center,
};

class Widget : public std::enable_shared_from_this<Widget> {
public:
    virtual ~Widget() = default;

    virtual void render();
    virtual bool dispatch(SDL_Keysym);
    virtual void handle_mousedown(SDL_MouseButtonEvent const&) { }
    virtual void handle_click(SDL_MouseButtonEvent const&) { }
    virtual void handle_wheel(SDL_MouseWheelEvent const&) { }
    virtual void handle_motion(SDL_MouseMotionEvent const&) { }
    virtual void handle_text_input() { }
    virtual void resize(Box const&);
    [[nodiscard]] virtual std::optional<ScheduledCommand> command(std::string const&) const;
    [[nodiscard]] virtual std::vector<ScheduledCommand> commands() const;

protected:
    Widget() = default;
    Commands* m_commands { nullptr };
};

enum class SizePolicy {
    Absolute = 0,
    Relative,
    Characters,
    Calculated,
    Stretch,
};

class WindowedWidget;
class WidgetContainer;

using Renderer = std::function<void(WindowedWidget*)>;
using KeyHandler = std::function<bool(WindowedWidget*, SDL_Keysym)>;
using MouseButtonHandler = std::function<void(WindowedWidget*, SDL_MouseButtonEvent const&)>;
using MouseWheelHandler = std::function<void(WindowedWidget*, SDL_MouseWheelEvent const&)>;
using MouseMotionHandler = std::function<void(WindowedWidget*, SDL_MouseMotionEvent const&)>;
using TextHandler = std::function<void(WindowedWidget*)>;
using SizeCalculator = std::function<int(WindowedWidget*)>;

class WindowedWidget : public Widget {
public:
    WindowedWidget(SizePolicy = SizePolicy::Stretch, int = 0);
    WindowedWidget(SizeCalculator);

    [[nodiscard]] virtual int height() const;
    [[nodiscard]] virtual int width() const;
    [[nodiscard]] virtual int top() const;
    [[nodiscard]] virtual int left() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] SizePolicy policy() const;
    [[nodiscard]] int policy_size() const;
    [[nodiscard]] WidgetContainer const* parent() const;

    [[nodiscard]] Position position() const;
    [[nodiscard]] Size size() const;
    [[nodiscard]] Box const& outline() const;
    void set_renderer(Renderer);
    void set_keyhandler(KeyHandler);
    void set_mousedownhandler(MouseButtonHandler);
    void set_mouseclickhandler(MouseButtonHandler);
    void set_mousewheelhandler(MouseWheelHandler);
    void set_mousemotionhandler(MouseMotionHandler);
    void set_texthandler(TextHandler);
    void set_size_calculator(SizeCalculator);
    void render() override;
    bool dispatch(SDL_Keysym) override;
    void handle_mousedown(SDL_MouseButtonEvent const&) override;
    void handle_click(SDL_MouseButtonEvent const&) override;
    void handle_wheel(SDL_MouseWheelEvent const&) override;
    void handle_motion(SDL_MouseMotionEvent const&) override;
    void handle_text_input() override;
    void resize(Box const&) override;
    int calculate_size();

    SDL_Rect render_text(int x, int y, std::string const& text,
        SDL_Color const& color = SDL_Color { 255, 255, 255, 255 },
        TextAlignment = TextAlignment::Left,
        SDLContext::SDLFontFamily = SDLContext::SDLFontFamily::Fixed) const;

    template <class Str>
    SDL_Rect render_fixed(int x, int y, Str text, SDL_Color const& color = SDL_Color { 255, 255, 255, 255 }) const
    {
        return render_text(x, y, std::string(text), color, TextAlignment::Left, SDLContext::SDLFontFamily::Fixed);
    }

    template <class Str>
    SDL_Rect render_fixed_right_aligned(int x, int y, Str text, SDL_Color const& color = SDL_Color { 255, 255, 255, 255 }) const
    {
        return render_text(x, y, std::string(text), color, TextAlignment::Left, SDLContext::SDLFontFamily::Fixed);
    }

    template <class Str>
    SDL_Rect render_fixed_centered(int y, Str text, SDL_Color const& color = SDL_Color { 255, 255, 255, 255 }) const
    {
        return render_text(0, y, std::string(text), color, TextAlignment::Center, SDLContext::SDLFontFamily::Fixed);
    }

    SDL_Rect normalize(SDL_Rect const&) const;
    void box(SDL_Rect const&, SDL_Color) const;
    void rectangle(SDL_Rect const&, SDL_Color) const;
    void roundedRectangle(SDL_Rect const&, int, SDL_Color) const;

protected:
private:
    friend class WidgetContainer;
    void set_parent(WidgetContainer *parent) { m_parent = parent; }

    SizePolicy m_policy { SizePolicy::Absolute };
    int m_size { 0 };
    Box m_outline;
    WidgetContainer* m_parent { nullptr };

    Renderer m_renderer { nullptr };
    KeyHandler m_keyhandler { nullptr };
    MouseButtonHandler m_mousedownhandler { nullptr };
    MouseButtonHandler m_mouseclickhandler { nullptr };
    MouseWheelHandler m_mousewheelhandler { nullptr };
    MouseMotionHandler m_mousemotionhandler { nullptr };
    TextHandler m_texthandler { nullptr };
    SizeCalculator m_size_calculator { nullptr };
};

enum class ContainerOrientation {
    Vertical = 0,
    Horizontal,
};

class WidgetContainer {
public:
    explicit WidgetContainer(ContainerOrientation);
    void add_component(WindowedWidget*);
    [[nodiscard]] std::vector<Widget*> components() const;
    void resize(Box const&);

    template <class ComponentClass>
    requires std::derived_from<ComponentClass, WindowedWidget>
    ComponentClass* get_component()
    {
        for (auto& c : components()) {
            if (auto casted = dynamic_cast<ComponentClass*>(c); casted != nullptr)
                return casted;
        }
        return nullptr;
    }

    void handle_mousedown(Box const&, SDL_MouseButtonEvent const&);
    void handle_click(Box const&, SDL_MouseButtonEvent const&);
    void handle_wheel(Box const&, SDL_MouseWheelEvent const&);
    void handle_motion(Box const&, SDL_MouseMotionEvent const&);

private:
    ContainerOrientation m_orientation { ContainerOrientation::Vertical };
    std::vector<std::unique_ptr<WindowedWidget>> m_components;
    std::vector<Box> m_outlines;
    Widget* m_mouse_focus { nullptr };
};

class Layout : public WindowedWidget {
public:
    Layout(ContainerOrientation, SizePolicy = SizePolicy::Stretch, int = 0);
    void render() override;
    bool dispatch(SDL_Keysym) override;
    void resize(Box const&) override;
    std::vector<Widget*> components();
    void add_component(WindowedWidget*);
    WidgetContainer const& container() const;
    void handle_mousedown(SDL_MouseButtonEvent const&) override;
    void handle_click(SDL_MouseButtonEvent const&) override;
    void handle_wheel(SDL_MouseWheelEvent const&) override;
    void handle_motion(SDL_MouseMotionEvent const&) override;
    std::optional<ScheduledCommand> command(std::string const&) const override;
    [[nodiscard]] std::vector<ScheduledCommand> commands() const override;

    template <class ComponentClass>
    requires std::derived_from<ComponentClass, WindowedWidget>
    ComponentClass* get_component()
    {
        return m_container.get_component<ComponentClass>();
    }

protected:
    WidgetContainer& container();

private:
    WidgetContainer m_container;
};

enum class FrameStyle {
    None,
    Rectangle,
    Rounded,
};

class Frame : public WindowedWidget {
public:
    Frame(FrameStyle, int, WindowedWidget*, SizePolicy = SizePolicy::Stretch, int = 0);
    void render() override;
    bool dispatch(SDL_Keysym) override;
    void resize(Box const&) override;
    [[nodiscard]] WindowedWidget* contents();
    [[nodiscard]] int margin() const;
    [[nodiscard]] int clamped_margin() const;
    [[nodiscard]] FrameStyle frame_style() const;

private:
    FrameStyle m_frame_style { FrameStyle::None };
    int m_margin { 3 };
    int m_clamped_margin { 3 };
    std::unique_ptr<WindowedWidget> m_contents;
};

class ModalWidget : public WindowedWidget {
public:
    ModalWidget(int, int);
    void dismiss();

    [[nodiscard]] int width() const override;
    [[nodiscard]] int height() const override;
    [[nodiscard]] int top() const override;
    [[nodiscard]] int left() const override;

private:
    int m_width;
    int m_height;
};

}
