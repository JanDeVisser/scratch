/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lexer/Token.h>
#include <Widget/Widget.h>

namespace Scratch {

struct Line {
    Line() = default;

    int start_index {0};
    std::vector<Token> tokens {};
};

struct DocumentPosition {
    int line { 0 };
    int column { 0 };

    void clear()
    {
        line = column = 0;
    }
    auto operator<=>(DocumentPosition const& other) const = default;
};

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
