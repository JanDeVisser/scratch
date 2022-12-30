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
    S(Document)              \
    S(Editor)                \
    S(Menu)                  \
    S(MenuBar)               \
    S(MenuEntry)             \
    S(Messenger)             \
    S(StatusBar)             \
    S(StatusReporter)        \
    S(WindowedWidget)        \
    S(Widget)

namespace Scratch {

#undef ENUM_CLASS
#define ENUM_CLASS(type)                   \
    class type;                            \
    using p##type = std::shared_ptr<type>; \
    using type##s = std::vector<p##type>;
ENUMERATE_CLASSES(ENUM_CLASS)
#undef ENUM_CLASS

using strings = std::vector<std::string>;

}
