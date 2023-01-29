/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Format.h>

#include <Gutter.h>
#include <Scratch.h>
#include <SDLContext.h>

namespace Scratch {

Gutter::Gutter()
    : WindowedWidget(SizePolicy::Absolute, 10 * Widget::char_width)
{
    set_renderer([](WindowedWidget* gutter) -> void {
        auto screen_top = Scratch::editor()->document()->screen_top();
        auto lines = Scratch::editor()->document()->line_count();
        auto rows = Scratch::editor()->rows();
        for (auto row = 0; row < rows && screen_top + row < lines; ++row) {
            auto x = 24;
            auto y = row * App::instance().context()->character_height() * 1.2;

            auto line = screen_top + row;
            auto color = PaletteIndex::LineNumber;
            if (line == Scratch::editor()->document()->point_line())
                color = PaletteIndex::ANSIBrightYellow;
            gutter->render_fixed(x, (int)y, Obelix::format("{>4}", line + 1),
                App::instance().color(color));
        }
    });
}

}
