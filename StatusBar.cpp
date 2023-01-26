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
    : WindowedWidget(SizePolicy::Absolute, App::instance().context()->character_height() + 3)
{
}

void StatusBar::render()
{
    std::vector<std::string> status = App::instance().status();
    for (auto const& c : App::instance().components()) {
        auto s = c->status();
        for (auto const& msg : s) {
            status.push_back(msg);
        }
    }
    auto x = width() - 8;
    for (auto const& msg : status) {
        auto rect = render_fixed_right_aligned(x, 2, msg);
        x = rect.x - 8;
    }
}

}
