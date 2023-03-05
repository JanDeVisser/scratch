/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Interp/Function.h>

namespace Scratch::Interp {
Function::Function(std::string name)
    : m_name(name)
{
}

std::string const& Function::name() const
{
    return m_name;
}

std::string Function::to_string() const
{
    return name();
}

}
