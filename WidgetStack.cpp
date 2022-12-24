/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <WidgetStack.h>

namespace Scratch {

void WidgetStack::add(pWidget page)
{
    m_widgets.push_back(page);
    if (m_widgets.size() == 1)
        m_selected = { m_widgets[m_current] };
}

void WidgetStack::next()
{
    if (!m_widgets.empty()) {
        m_current = (m_current + 1) % static_cast<int>(m_widgets.size());
        m_selected = { m_widgets[m_current] };
    }
}

void WidgetStack::previous()
{
    if (!m_widgets.empty()) {
        if (--m_current < 0) {
            m_current = static_cast<int>(m_widgets.size()) - 1;
        }
        m_selected = { m_widgets[m_current] };
    }
}

Widgets const& WidgetStack::components()
{
    return m_selected;
}

bool WidgetStack::handle(int key)
{
    switch (key) {
    case ctrl('n'):
        next();
        return true;
    case ctrl('p'):
        previous();
        return true;
    default:
        return false;
    }
}

}
