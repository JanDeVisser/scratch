/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <App.h>

namespace Scratch {

class StatusBar : public WindowedWidget {
public:
    explicit StatusBar();
    void render() override;
};

}
