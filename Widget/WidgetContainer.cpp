/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <Widget/Widget.h>

namespace Scratch {

WidgetContainer::WidgetContainer(ContainerOrientation orientation)
    : m_orientation(orientation)
{
}

void WidgetContainer::add_component(WindowedWidget* widget)
{
    widget->set_parent(this);
    m_components.emplace_back(widget);
}

std::vector<Widget*> WidgetContainer::components() const
{
    std::vector<Widget*> ret;
    ret.reserve(m_components.size());
    for (auto const& c : m_components) {
        ret.push_back(c.get());
    }
    return ret;
}

void WidgetContainer::resize(Box const& outline)
{
    debug(scratch, "Resizing container within outline '{}'", outline);
    m_outlines.clear();
    for (auto ix = 0; ix < m_components.size(); ++ix)
        m_outlines.emplace_back();
    auto allocated = 0;
    auto stretch_count = 0;
    auto total = (m_orientation == ContainerOrientation::Vertical) ? outline.height() : outline.width();
    auto fixed_size = (m_orientation == ContainerOrientation::Vertical) ? outline.width() : outline.height();
    auto fixed_size_coord = (m_orientation == ContainerOrientation::Vertical) ? 0 : 1;
    auto var_size_coord = (m_orientation == ContainerOrientation::Vertical) ? 1 : 0;
    auto fixed_pos = (m_orientation == ContainerOrientation::Vertical) ? outline.left() : outline.top();
    auto fixed_pos_coord = (m_orientation == ContainerOrientation::Vertical) ? 0 : 1;
    auto var_pos_coord = (m_orientation == ContainerOrientation::Vertical) ? 1 : 0;

    for (auto ix = 0; ix < m_components.size(); ++ix) {
        auto const& c = m_components[ix];
        m_outlines[ix].size[fixed_size_coord] = fixed_size;
        m_outlines[ix].position[fixed_pos_coord] = fixed_pos;
        int sz = 0;
        switch (c->policy()) {
        case SizePolicy::Absolute:
            sz = c->policy_size();
            break;
        case SizePolicy::Relative: {
            sz = (int)((float)(total * c->policy_size()) / 100.0f);
        } break;
        case SizePolicy::Characters: {
            sz = c->policy_size() * ((m_orientation == ContainerOrientation::Vertical) ? App::instance().context()->character_height() : App::instance().context()->character_width());
        } break;
        case SizePolicy::Calculated: {
            sz = c->calculate_size();
        } break;
        case SizePolicy::Stretch: {
            sz = -1;
            stretch_count++;
        } break;
        }
        oassert(sz != 0, "Size Policy {} resulted in zero size", (int)c->policy());
        m_outlines[ix].size[var_size_coord] = sz;
        if (sz > 0)
            allocated += sz;
    }

    if (stretch_count) {
        oassert(total > allocated, "No room left in container for {} stretched components. Available: {} Allocated: {}", stretch_count, total, allocated);
        auto stretch = (int)((float)(total - allocated) / (float)stretch_count);
        for (auto& o : m_outlines) {
            if (o.size[var_size_coord] == -1)
                o.size[var_size_coord] = stretch;
        }
    }

    auto offset = 0;
    for (auto ix = 0; ix < m_components.size(); ++ix) {
        auto& o  = m_outlines[ix];
        o.position[var_pos_coord] = offset;
        offset += o.size[var_size_coord];
        m_components[ix]->resize(o);
        debug(scratch, "Component {}: '{}'", ix, o);
    }
}

void WidgetContainer::handle_motion(Box const& outline, SDL_MouseMotionEvent const& event)
{
    m_mouse_focus = nullptr;
    if (outline.contains((int) event.x, (int) event.y)) {
        for (auto const& c : m_components) {
            if (c->outline().contains((int) event.x, (int) event.y)) {
                m_mouse_focus = c.get();
                c->handle_motion(event);
                return;
            }
        }
    }
}

void WidgetContainer::handle_mousedown(Box const& outline, SDL_MouseButtonEvent const& event)
{
    if (outline.contains((int) event.x, (int) event.y)) {
        for (auto const& c : m_components) {
            if (c->outline().contains((int) event.x, (int) event.y)) {
                c->handle_mousedown(event);
                return;
            }
        }
    }
}

void WidgetContainer::handle_click(Box const& outline, SDL_MouseButtonEvent const& event)
{
    if (outline.contains((int) event.x, (int) event.y)) {
        for (auto const& c : m_components) {
            if (c->outline().contains((int) event.x, (int) event.y)) {
                c->handle_click(event);
                return;
            }
        }
    }
}

void WidgetContainer::handle_wheel(Box const& outline, SDL_MouseWheelEvent const& event)
{
    Position mouse = App::instance().mouse_position();
    if (outline.contains(mouse.left(), mouse.top())) {
        for (auto const& c : m_components) {
            if (c->outline().contains(mouse.left(), mouse.top())) {
                c->handle_wheel(event);
                return;
            }
        }
    }
}

}
