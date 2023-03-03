/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define ENUMERATE_CLASSES(S) \
    S(App)                   \
    S(Command)               \
    S(Commands)              \
    S(Document)              \
    S(Editor)                \
    S(Gutter)                \
    S(Layout)                \
    S(ModalWidget)           \
    S(StatusBar)             \
    S(Widget)                \
    S(WidgetContainer)       \
    S(WindowedWidget)

namespace Scratch {

#undef ENUM_CLASS
#define ENUM_CLASS(type) \
    class type;
ENUMERATE_CLASSES(ENUM_CLASS)
#undef ENUM_CLASS

using strings = std::vector<std::string>;

}
