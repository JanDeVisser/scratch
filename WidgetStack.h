/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Widget.h>

namespace Scratch {

class WidgetStack : public Widget {
public:
    WidgetStack() = default;
    void add(pWidget);
    void next();
    void previous();
    [[nodiscard]] Widgets const& components() override;
    bool handle(int) override;
private:
    Widgets m_widgets;
    Widgets m_selected;
    int m_current { 0 };
};

}
