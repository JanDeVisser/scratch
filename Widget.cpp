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

int Widget::char_height { 0 };
int Widget::char_width { 0 };

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
    auto ret = App::instance().context()->render_fixed(left() + x, top() + y,
        text, color);
    ret.x -= left();
    ret.y -= top();
    return ret;
}

SDL_Rect Widget::render_fixed_right_aligned(int x, int y, std::string const& text, SDL_Color const& color) const
{
    auto ret = App::instance().context()->render_fixed_right_aligned(left() + x, top() + y,
        text, color);
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

void WindowedWidget::handle_text_input()
{
    if (m_texthandler != nullptr)
        m_texthandler(this);
}

void WindowedWidget::resize(Box const& outline)
{
    m_outline = outline;
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
        auto const& c  = m_components[ix];
        m_outlines[ix].size[fixed_size_coord] = fixed_size;
        m_outlines[ix].position[fixed_pos_coord] = fixed_pos;
        switch (c->policy()) {
        case SizePolicy::Absolute:
            m_outlines[ix].size[var_size_coord] = c->policy_size();
            allocated += c->policy_size();
            break;
        case SizePolicy::Relative: {
            auto sz = (int)((float)(total * c->policy_size()) / 100.0f);
            m_outlines[ix].size[var_size_coord] = sz;
            allocated += sz;
        } break;
        case SizePolicy::Stretch:
            m_outlines[ix].size[var_size_coord] = -1;
            stretch_count++;
            break;
        }
    }

    if (stretch_count) {
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
