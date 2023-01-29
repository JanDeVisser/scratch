/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <App.h>

namespace Scratch {

class StatusBar : public Layout {
public:
    explicit StatusBar();
    void add_applet(int, Renderer);
};

}
