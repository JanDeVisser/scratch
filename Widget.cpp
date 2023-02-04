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

void Widget::render()
{
}

void Widget::resize(Box const&)
{
}

int Widget::top() const
{
    return 0;
}

int Widget::left() const
{
    return 0;
}

bool Widget::empty() const
{
    return width() == 0 && height() == 0;
}

SDL_Rect Widget::render_fixed(int x, int y, std::string const& text, SDL_Color const& color) const
{
    auto ret = App::instance().context()->render_text(left() + x, top() + y,
        text, color, SDLContext::SDLFontFamily::Fixed);
    ret.x -= left();
    ret.y -= top();
    return ret;
}

SDL_Rect Widget::render_fixed_right_aligned(int x, int y, std::string const& text, SDL_Color const& color) const
{
    auto ret = App::instance().context()->render_text_right_aligned(left() + x, top() + y,
        text, color, SDLContext::SDLFontFamily::Fixed);
    ret.x -= left();
    ret.y -= top();
    return ret;
}

SDL_Rect Widget::render_fixed_centered(int y, std::string const& text, SDL_Color const& color) const
{
    auto ret = App::instance().context()->render_text_centered(left() + width()/2, top() + y,
        text, color, SDLContext::SDLFontFamily::Fixed);
    ret.x -= left();
    ret.y -= top();
    return ret;
}

SDL_Rect Widget::normalize(SDL_Rect const& rect) const
{
    SDL_Rect r = rect;
    if (r.x < 0)
        r.x = width() + r.x;
    if (r.y < 0)
        r.y = height() + r.y;
    if (r.w <= 0)
        r.w = width() - r.x + r.w;
    if (r.h <= 0)
        r.h = height() - r.y + r.h;
    r.x = clamp(r.x, 0, width());
    r.y = clamp(r.y, 0, height());
    r.w = clamp(r.w, 0, width() - r.x);
    r.h = clamp(r.h, 0, height() - r.y);
    return r;
}

void Widget::box(SDL_Rect const& rect, SDL_Color color) const
{
    auto r = normalize(rect);
    boxColor(
        App::instance().renderer(),
        left() + r.x,
        top() + r.y,
        left() + r.x + r.w,
        top() + r.y + r.h,
        *((uint32_t*)&color));
}

void Widget::rectangle(SDL_Rect const& rect, SDL_Color color) const
{
    auto r = normalize(rect);
    rectangleColor(
        App::instance().renderer(),
        left() + r.x,
        top() + r.y,
        left() + r.x + r.w,
        top() + r.y + r.h,
        *((uint32_t*)&color));
}

void Widget::roundedRectangle(SDL_Rect const& rect, int radius, SDL_Color color) const
{
    auto r = normalize(rect);
    roundedRectangleColor(
        App::instance().renderer(),
        left() + r.x,
        top() + r.y,
        left() + r.x + r.w,
        top() + r.y + r.h,
        radius,
        *((uint32_t*)&color));
}

/* ----------------------------------------------------------------------- */

WindowedWidget::WindowedWidget(SizePolicy policy, int size)
    : Widget()
    , m_policy(policy)
    , m_size(size)
{
}

WindowedWidget::WindowedWidget(SizeCalculator calculator)
    : Widget()
    , m_policy(SizePolicy::Calculated)
    , m_size_calculator(std::move(calculator))
{
}

SizePolicy WindowedWidget::policy() const
{
    return m_policy;
}

int WindowedWidget::policy_size() const
{
    return m_size;
}

WidgetContainer const* WindowedWidget::parent() const
{
    return m_parent;
}

Position WindowedWidget::position() const
{
    return m_outline.position;
}

Size WindowedWidget::size() const
{
    return m_outline.size;
}

int WindowedWidget::top() const
{
    return position().top();
}

int WindowedWidget::left() const
{
    return position().left();
}

int WindowedWidget::height() const
{
    return size().height();
}

int WindowedWidget::width() const
{
    return size().width();
}

Box const& WindowedWidget::outline() const
{
    return m_outline;
}

void WindowedWidget::set_renderer(Renderer renderer)
{
    m_renderer = std::move(renderer);
}

void WindowedWidget::set_keyhandler(KeyHandler handler)
{
    m_keyhandler = std::move(handler);
}

void WindowedWidget::set_texthandler(TextHandler handler)
{
    m_texthandler = std::move(handler);
}

void WindowedWidget::set_size_calculator(SizeCalculator calculator)
{
    m_size_calculator = std::move(calculator);
    m_policy = SizePolicy::Calculated;
}

void WindowedWidget::render()
{
    if (m_renderer != nullptr)
        m_renderer(this);
}

bool WindowedWidget::dispatch(SDL_Keysym sym)
{
    if (m_keyhandler != nullptr)
        return m_keyhandler(this, sym);
    return false;
}

void WindowedWidget::handle_click(SDL_MouseButtonEvent const& event)
{
    if (m_mousebuttonhandler != nullptr)
        m_mousebuttonhandler(this, event);
}

void WindowedWidget::handle_text_input()
{
    if (m_texthandler != nullptr)
        m_texthandler(this);
}

void WindowedWidget::resize(Box const& outline)
{
    m_outline = outline;
}

int WindowedWidget::calculate_size()
{
    assert(m_policy == SizePolicy::Calculated && m_size_calculator != nullptr);
    return m_size_calculator(this);
}

/* ----------------------------------------------------------------------- */

WidgetContainer::WidgetContainer(ContainerOrientation orientation)
    : m_orientation(orientation)
{
}

void WidgetContainer::add_component(WindowedWidget* widget)
{
    widget->set_parent(this);
    m_components.emplace_back(widget);
}

std::vector<Widget*> WidgetContainer::components()
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

/* ----------------------------------------------------------------------- */

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
    for (auto* c : m_container.components()) {
        if (c->dispatch(sym))
            return true;
    }
    return false;
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

void Layout::handle_click(SDL_MouseButtonEvent const& event)
{
    m_container.handle_click(outline(), event);
}

/* ----------------------------------------------------------------------- */

ModalWidget::ModalWidget(int w, int h)
    : m_width(w)
    , m_height(h)
{
}

int ModalWidget::width() const
{
    return m_width;
}

int ModalWidget::height() const
{
    return m_height;
}

int ModalWidget::top() const
{
    return (App::instance().height() - height()) / 2;
}

int ModalWidget::left() const
{
    return (App::instance().width() - width()) / 2;
}

void ModalWidget::dismiss()
{
    if (App::instance().modal() == this)
        App::instance().dismiss_modal();
}

}
