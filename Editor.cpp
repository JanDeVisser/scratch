/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <cctype>
#include <filesystem>
#include <sstream>

#include <SDL2_gfxPrimitives.h>

#include <App.h>
#include <Editor.h>
#include <SDLContext.h>

namespace Scratch {

extern_logging_category(scratch);

namespace fs=std::filesystem;

Editor::Editor()
    : WindowedWidget(
        Vector<int, 4> { 0, 0, 0, -(2 * App::instance().context()->character_height() + 6) },
        Vector<int, 4> { 8, 8, 18, 0 })
{
    m_rows = content_height() / (int)(App::instance().context()->character_height() * 1.2);
    debug(scratch, "--- content height: {} character height: {} rows: {}", content_height(), App::instance().context()->character_height(), m_rows);
    m_columns = content_width() / App::instance().context()->character_width();
    debug(scratch, "--- content width: {} character width: {} rows: {}", content_width(), App::instance().context()->character_width(), m_columns);
}

int Editor::rows() const
{
    return m_rows;
}

int Editor::columns() const
{
    return m_columns;
}

int Editor::line_top(int line) const
{
    return App::instance().context()->character_height() * line * 1.2;
}

int Editor::line_bottom(int line) const
{
    return App::instance().context()->character_height() * (line + 1) * 1.2;
}

int Editor::column_left(int column) const
{
    return column_width() * column;
}

int Editor::column_right(int column) const
{
    return column_width() * (column + 1);
}

int Editor::line_height() const
{
    return App::instance().context()->character_height();
}

int Editor::column_width() const
{
    return App::instance().context()->character_width();
}

void Editor::render()
{
    box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
    m_line = 0;
    m_column = 0;
    document().render(this);
}

void Editor::mark_current_line(int line)
{
    SDL_Rect r {
        0,
        line_top(line),
        0,
        line_height()
    };
    box(r, App::instance().color(PaletteIndex::CurrentLineFill));
    rectangle(r, App::instance().color(PaletteIndex::CurrentLineEdge));
}

void Editor::text_cursor(int line, int column)
{
    if (App::instance().modal() != nullptr)
        return;
    static auto time_start = std::chrono::system_clock::now();
    auto time_end = std::chrono::system_clock::now();
    const long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
    if (elapsed > 400) {
        //        Clipper clipCode(App::instance().renderer(), rectCode);

        SDL_Rect r {
            column_left(column),
            line_top(line),
            1,
            line_height()
        };
        box(r, App::instance().color(PaletteIndex::Cursor));
        if (elapsed > 800)
            time_start = time_end;
    }
}

void Editor::append(DisplayToken const& token)
{
    auto x = m_column * App::instance().context()->character_width();
    auto y = m_line * App::instance().context()->character_height() * 1.2;
    render_fixed(x, (int)y, token.text, App::instance().color(token.color));
    m_column += (int)token.text.length();
}

void Editor::newline()
{
    ++m_line;
    m_column = 0;
}

 bool Editor::dispatch(SDL_Keysym sym)
{
     return document().dispatch(this, sym);
 }

 void Editor::handle_text_input()
 {
     document().handle_text_input();
 }

std::string Editor::open_file(std::string const& file_name)
{
    //    if (!document().filename().empty() || (document().line_count() > 0)) {
    //        m_documents.emplace_back();
    //        m_current_document = m_documents.back();
    //    }
    return document().load(file_name);
}

std::string Editor::save_file()
{
    if (!document().filename().empty() || (document().line_count() > 0))
        return document().save();
    return "";
}

std::vector<std::string> Editor::status()
{
    std::vector<std::string> ret;
    std::stringstream ss;
    ss << document().point_line() + 1 << ":" << document().point_column() + 1;
    ret.push_back(ss.str());
    ret.push_back(fs::relative(document().filename()).string());
    return ret;
}

}
