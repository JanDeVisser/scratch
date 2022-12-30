/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Document.h>
#include <StatusBar.h>
#include <Widget.h>

namespace Scratch {

class Editor : public WindowedWidget, public StatusReporter {
public:
    Editor();

    [[nodiscard]] int screen_top() const { return m_screen_top; }
    [[nodiscard]] int screen_left() const { return m_screen_left; }
    [[nodiscard]] int point_line() const { return m_point_line; }
    [[nodiscard]] int point_column() const { return m_point_column; }
    [[nodiscard]] int virtual_point_column() const { return m_virtual_point_column; }

    [[nodiscard]] Document& document() { return m_document; }
    std::string status() override;

    void render() override;
    void post_render() override;
    [[nodiscard]] bool handle(KeyCode) override;
private:
    Document m_document {};

    size_t m_screen_top {0};
    size_t m_screen_left {0};
    size_t m_point_line {0};
    size_t m_point_column {0};
    size_t m_virtual_point_column {0};
};

}
