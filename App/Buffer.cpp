/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App/Buffer.h>
#include <App/Editor.h>

namespace Scratch {

Buffer::Buffer(Editor* editor)
    : m_editor(editor)
{
}

std::string Buffer::short_title() const
{
    return title();
}

int Buffer::rows() const
{
    return m_editor->rows();
}

int Buffer::columns() const
{
    return m_editor->columns();
}

} // Scratch
