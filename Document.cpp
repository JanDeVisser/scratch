/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>
#include <unistd.h>

#include <Document.h>

namespace Scratch {

std::string const& Document::line(size_t line_no) const
{
    assert(line_no < m_lines.size());
    return m_lines[line_no];
}

size_t Document::line_length(size_t line_no) const
{
    assert(line_no < m_lines.size());
    return m_lines[line_no].length();
}

size_t Document::line_count() const
{
    return m_lines.size();
}

void Document::backspace(size_t column, size_t row)
{
    assert(row < m_lines.size());
    assert(column <= m_lines[row].length());
    assert(column > 0 || row > 0);
    if (column > 0) {
        m_lines[row].erase(column - 1, 1);
    } else {
        join_lines(row-1);
    }
}

void Document::split_line(size_t column, size_t row)
{
    if (row >= m_lines.size()) {
        m_lines.emplace_back("");
        return;
    }
    auto right = m_lines[row].substr(column);
    m_lines[row].erase(column);
    auto iter = m_lines.begin();
    for (auto ix = 0u; ix < row; ++ix, ++iter);
    m_lines.insert(iter, right);
}

void Document::join_lines(size_t row)
{
    assert(row < m_lines.size()-1);
    m_lines[row] += m_lines[row+1];
    auto iter = m_lines.begin();
    for (auto ix = 0u; ix < row+1; ++ix, ++iter);
    m_lines.erase(iter);
}

void Document::insert(size_t column, size_t row, char ch)
{
    assert(row < m_lines.size());
    assert(column <= m_lines[row].length());
    m_lines[row].insert(column, std::string() + ch);
}

void Document::clear()
{
    m_lines.clear();
}

std::string Document::load(std::string file_name)
{
    char file_buffer[64 * 1024];

    m_filename = std::move(file_name);
    clear();

    int fh = ::open(m_filename.c_str(), O_RDONLY);
    if (fh < 0)
        return strerror(errno);

    std::string current;
    for (;;) {
        auto read_bytes = ::read(fh, file_buffer, 64*1024);
        if (read_bytes < 0) {
            ::close(fh);
            return strerror(errno);
        }
        m_size += read_bytes;
        for (auto ix = 0u; ix < read_bytes; ++ix) {
            char ch = file_buffer[ix];
            switch (ch) {
            case '\r':
                break;
            case '\n': {
                m_lines.push_back(current);
                current.clear();
                break;
            }
            default: {
                current += ch;
                break;
            }
            }
        }
        if (read_bytes < 64*1024) {
            m_lines.push_back(current);
            ::close(fh);
            m_dirty = false;
            return "";
        }
    }
}

std::string Document::save()
{
    return "";
}

std::string Document::save_as(std::string)
{
    return "";
}

std::string const& Document::filename() const
{
    return m_filename;
}

}
