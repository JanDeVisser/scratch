/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <vector>

#include <Scribble/Syntax/SyntaxNodeType.h>
#include <Scribble/Syntax/Syntax.h>

namespace Scratch::Scribble {

#undef ENUM_SYNTAXNODETYPE
#define ENUM_SYNTAXNODETYPE(type)          \
    class type;                            \
    using p##type = std::shared_ptr<type>; \
    using type##s = std::vector<p##type>;
ENUMERATE_SYNTAXNODETYPES(ENUM_SYNTAXNODETYPE)
#undef ENUM_SYNTAXNODETYPE

}
