/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Editor.h>
#include <SDLContext.h>
#include <StatusBar.h>

using namespace Obelix;

namespace Scratch {

StatusBar::StatusBar()
    : Layout(ContainerOrientation::Horizontal, SizePolicy::Absolute, App::instance().context()->character_height() + 9)
{
}

void StatusBar::add_applet(int sz, Renderer renderer)
{
    auto widget = new WindowedWidget(SizePolicy::Absolute, sz);
    widget->set_renderer(std::move(renderer));
    auto frame = new Frame(FrameStyle::Rectangle, 3, widget, SizePolicy::Absolute, sz + 6);
    add_component(frame);
}

}
