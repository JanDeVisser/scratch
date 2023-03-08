/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Widget/Widget.h>

namespace Scratch {

class Buffer : public Widget {
public:
    [[nodiscard]] int rows() const;
    [[nodiscard]] int columns() const;
    [[nodiscard]] virtual std::string title() const = 0;
    [[nodiscard]] virtual std::string short_title() const;
    [[nodiscard]] virtual std::string status() const = 0;
    virtual void on_activate() { };
    virtual void on_deactivate() { };
    virtual void mousedown(int, int) { };
    virtual void motion(int, int) { };
    virtual void click(int, int, int) { };
    virtual void wheel(int) { };
protected:
    explicit Buffer(Editor*);
    [[nodiscard]] Editor* editor() { return m_editor; }
private:
    Editor* m_editor;
};

} // Scratch
