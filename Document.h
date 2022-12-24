/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Forward.h>

namespace Scratch {

class Document {
public:
    Document() = default;

    [[nodiscard]] std::string const& line(size_t) const;
    [[nodiscard]] size_t line_length(size_t) const;
    [[nodiscard]] size_t line_count() const;
    [[nodiscard]] std::string const& filename() const;

    void backspace(size_t column, size_t row);
    void split_line(size_t column, size_t row);
    void insert(size_t column, size_t row, char);
    void join_lines(size_t);

    void clear();
    std::string load(std::string);
    std::string save();
    std::string save_as(std::string);
    [[nodiscard]] bool dirty() const { return m_dirty; }

private:
    std::string m_filename;
    strings m_lines;
    size_t m_size { 0 };
    bool m_dirty { false };
};

}
