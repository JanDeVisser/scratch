/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Commands/Command.h>
#include <Widget/Widget.h>

namespace Scratch {

Layout::Layout(ContainerOrientation orientation, SizePolicy policy, int size)
    : WindowedWidget(policy, size)
    , m_container(orientation)
{
}

void Layout::render()
{
    for (auto* c : m_container.components()) {
        c->render();
    }
}

bool Layout::dispatch(SDL_Keysym sym)
{
    if (Widget::dispatch(sym))
        return true;
    for (auto* c : m_container.components()) {
        if (c->dispatch(sym))
            return true;
    }
    return false;
}

std::optional<ScheduledCommand> Layout::command(std::string const& name) const
{
    if (auto ret = Widget::command(name); ret.has_value())
        return ret;
    for (auto* c : m_container.components()) {
        if (auto ret = c->command(name); ret.has_value())
            return ret;
    }
    return {};
}

std::vector<ScheduledCommand> Layout::commands() const
{
    std::vector<ScheduledCommand> ret;
    for (auto* c : m_container.components()) {
        auto component_commands = c->commands();
        for (auto const& cmd : component_commands) {
            ret.push_back(cmd);
        }
    }
    return ret;
}

void Layout::resize(Box const& outline)
{
    WindowedWidget::resize(outline);
    m_container.resize(outline);
}

std::vector<Widget*> Layout::components()
{
    return m_container.components();
}

void Layout::add_component(WindowedWidget* widget)
{
    m_container.add_component(widget);
}

WidgetContainer const& Layout::container() const
{
    return m_container;
}

WidgetContainer& Layout::container()
{
    return m_container;
}

void Layout::handle_mousedown(SDL_MouseButtonEvent const& event)
{
    m_container.handle_mousedown(outline(), event);
}

void Layout::handle_click(SDL_MouseButtonEvent const& event)
{
    m_container.handle_click(outline(), event);
}

void Layout::handle_wheel(SDL_MouseWheelEvent const& event)
{
    m_container.handle_wheel(outline(), event);
}

void Layout::handle_motion(SDL_MouseMotionEvent const& event)
{
    m_container.handle_motion(outline(), event);
}

}
