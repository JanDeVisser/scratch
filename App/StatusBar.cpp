/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Widget/SDLContext.h"
#include <App/Editor.h>
#include <App/StatusBar.h>

using namespace Obelix;

namespace Scratch {

StatusBar::StatusBar()
    : Layout(ContainerOrientation::Horizontal, SizePolicy::Calculated, App::instance().context()->character_height() + 12)
{
    set_size_calculator([](WindowedWidget*) -> int {
        return App::instance().context()->character_height() + 12;
    });
    add_component(new WindowedWidget());
}

Frame* StatusBar::add_applet(int sz, Renderer renderer)
{
    auto widget = new WindowedWidget(SizePolicy::Stretch);
    widget->set_renderer(std::move(renderer));
    auto frame = new Frame(FrameStyle::Rectangle, 3, widget, SizePolicy::Characters, sz + 1);
    add_component(frame);
    return frame;
}

}
