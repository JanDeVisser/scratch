/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

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
    : WindowedWidget()
{
    m_documents.emplace_back(new Document(this));
    m_current_document = m_documents.front().get();
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

int Editor::line_height()
{
    return App::instance().context()->character_height() * 1.2;
}

int Editor::column_width()
{
    return App::instance().context()->character_width();
}

void Editor::resize(const Box& outline)
{
    WindowedWidget::resize(outline);
    m_rows = height() / (int)(App::instance().context()->character_height() * 1.2);
    m_columns = width() / App::instance().context()->character_width();
}

void Editor::render()
{
    box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
    m_line = 0;
    m_column = 0;
    document()->render(this);
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
    return document()->dispatch(this, sym);
}

void Editor::handle_text_input()
{
    document()->handle_text_input();
}

void Editor::new_buffer()
{
    m_documents.emplace_back(new Document(this));
    m_current_document = m_documents.back().get();
}

std::string Editor::open_file(fs::path const& path)
{
    if (!document()->path().empty() || !document()->empty()) {
        new_buffer();
    }
    return document()->load(path);
}

std::string Editor::save_file()
{
    if (!document()->path().empty() || (document()->line_count() > 0))
        return document()->save();
    return "";
}

std::string Editor::save_file_as(fs::path const& new_file_name)
{
    if (!document()->path().empty() || (document()->line_count() > 0))
        return document()->save_as(new_file_name);
    return "";
}

std::string Editor::save_all()
{
    for (auto* doc : documents()) {
        if (doc->path().empty())
            /* App::instance().schedule("query-filename-and-save") */;
        else
            doc->save();
    }
    return "";
}

void Editor::move_to(int line, int column)
{
    document()->move_to(line, column);
}

void Editor::switch_to(std::string const& buffer_name)
{
    auto* buffer = document(buffer_name);
    if (buffer != nullptr) {
        m_current_document = buffer;
    }
}

std::vector<std::string> Editor::status()
{
    std::vector<std::string> ret;
    std::stringstream ss;
    ss << document()->point_line() + 1 << ":" << document()->point_column() + 1;
    ret.push_back(ss.str());
    ret.push_back(fs::relative(document()->path()).string());
    return ret;
}

std::vector<Document*> Editor::documents() const
{
    std::vector<Document*> ret;
    for (auto& doc : m_documents)
        ret.push_back(doc.get());
    std::sort(ret.begin(), ret.end(),
        [](auto const* d1, auto const* d2) {
            return d1->path() < d2->path();
        });
    return ret;
}

Document* Editor::document(fs::path const& path) const
{
    fs::path abs = fs::absolute(path);
    for (auto const& doc : m_documents) {
        if (abs == doc->path())
            return doc.get();
    }
    return nullptr;
}

}
