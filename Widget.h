/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <functional>

#include <SDL.h>

#include <Forward.h>
#include <Geometry.h>

namespace Scratch {

class Widget : public std::enable_shared_from_this<Widget> {
public:
    virtual ~Widget() = default;
    [[nodiscard]] virtual int height() const = 0;
    [[nodiscard]] virtual int width() const = 0;
    [[nodiscard]] virtual int top() const = 0;
    [[nodiscard]] virtual int left() const = 0;
    [[nodiscard]] bool empty() const;

    virtual void render();
    virtual bool dispatch(SDL_Keysym) { return false; }
    virtual void handle_text_input() { }
    virtual void resize(Box const&);
    virtual std::vector<std::string> status() { return {}; }

    SDL_Rect render_fixed(int, int, std::string const&, SDL_Color const& = SDL_Color { 255, 255, 255, 255 }) const;
    SDL_Rect render_fixed_right_aligned(int, int, std::string const&, SDL_Color const& = SDL_Color { 255, 255, 255, 255 }) const;

    SDL_Rect normalize(SDL_Rect const&) const;
    void box(SDL_Rect const&, SDL_Color) const;
    void rectangle(SDL_Rect const&, SDL_Color) const;
    void roundedRectangle(SDL_Rect const&, int, SDL_Color) const;

    static int char_height;
    static int char_width;
protected:
    Widget() = default;

private:
};

enum class SizePolicy {
    Absolute = 0,
    Relative,
    Stretch,
};

class WindowedWidget;
class WidgetContainer;

using Renderer = std::function<void(WindowedWidget*)>;
using KeyHandler = std::function<bool(WindowedWidget*, SDL_Keysym)>;
using TextHandler = std::function<void(WindowedWidget*)>;

class WindowedWidget : public Widget {
public:
    WindowedWidget(SizePolicy = SizePolicy::Stretch, int = 0);

    [[nodiscard]] int height() const override;
    [[nodiscard]] int width() const override;
    [[nodiscard]] int top() const override;
    [[nodiscard]] int left() const override;
    [[nodiscard]] SizePolicy policy() const;
    [[nodiscard]] int policy_size() const;
    [[nodiscard]] WidgetContainer const* parent() const;

    [[nodiscard]] Position position() const;
    [[nodiscard]] Size size() const;
    [[nodiscard]] Box const& outline() const;
    void set_renderer(Renderer);
    void set_keyhandler(KeyHandler);
    void set_texthandler(TextHandler);
    void render() override;
    bool dispatch(SDL_Keysym) override;
    void handle_text_input() override;
    void resize(Box const&) override;

protected:
private:
    friend class WidgetContainer;
    void set_parent(WidgetContainer *parent) { m_parent = parent; }

    SizePolicy m_policy { SizePolicy::Absolute };
    int m_size;
    Box m_outline;
    WidgetContainer* m_parent { nullptr };

    Renderer m_renderer { nullptr };
    KeyHandler m_keyhandler { nullptr };
    TextHandler m_texthandler { nullptr };
};

enum class ContainerOrientation {
    Vertical = 0,
    Horizontal,
};

class WidgetContainer {
public:
    explicit WidgetContainer(ContainerOrientation);
    void add_component(WindowedWidget*);
    [[nodiscard]] std::vector<Widget*> components();
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

private:
    ContainerOrientation m_orientation { ContainerOrientation::Vertical };
    std::vector<std::unique_ptr<WindowedWidget>> m_components;
    std::vector<Box> m_outlines;
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

class ModalWidget : public Widget {
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
