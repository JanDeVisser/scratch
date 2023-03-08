/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>
#include <sstream>

#include <SDL2_gfxPrimitives.h>

#include "Widget/App.h"
#include "Widget/SDLContext.h"
#include <App/Editor.h>
#include <App/Scratch.h>

namespace Scratch {

extern_logging_category(scratch);

namespace fs=std::filesystem;

EditorCommands::EditorCommands()
{
    register_command({ "new-buffer", "New buffer", {},
        [](Widget&, strings const&) -> void {
                             Scratch::editor()->new_file();
        }
    }, { SDLK_n, KMOD_CTRL });
    register_command({ "open-file", "Open file",
        {
            { "File to open", CommandParameterType::ExistingFilename }
        },
        [](Widget&, strings const& args) -> void {
            Scratch::editor()->open_file(args[0]);
        }
    }, { SDLK_o, KMOD_CTRL });
    register_command({ "save-all-files", "Save call files", {},
        [](Widget&, strings const&) -> void {
            Scratch::editor()->save_all();
        }
    }, { SDLK_l, KMOD_CTRL });
    register_command({ "switch-buffer", "Switch buffer",
        {
            { "Buffer", CommandParameterType::Buffer }
        },
        [](Widget&, strings const& args) -> void {
            Scratch::editor()->switch_to(args[0]);
        }
    }, { SDLK_b, KMOD_CTRL });
}

EditorCommands Editor::s_editor_commands;

Editor::Editor()
    : WindowedWidget()
{
    m_buffers.emplace_back(new Document(this));
    m_current_buffer = m_buffers.front().get();
    m_current_buffer->on_activate();

    Scratch::status_bar()->add_applet(20, [this](WindowedWidget* applet) -> void {
        applet->render_fixed_centered(2, buffer()->status(), SDL_Color { 0xff, 0xff, 0xff, 0xff });
    });
    Scratch::status_bar()->add_applet(20, [this](WindowedWidget* applet) -> void {
        applet->render_fixed(10, 2, buffer()->short_title(), SDL_Color { 0xff, 0xff, 0xff, 0xff });
    });
    m_commands = &s_editor_commands;
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
    return line * line_height();
}

int Editor::line_bottom(int line) const
{
    return (line + 1) * line_height();
}

int Editor::column_left(int column)
{
    return column_width() * column;
}

int Editor::column_right(int column)
{
    return column_width() * (column + 1);
}

int Editor::line_height() const
{
    return m_line_height;
}

int Editor::column_width()
{
    return App::instance().context()->character_width();
}

void Editor::resize(const Box& outline)
{
    WindowedWidget::resize(outline);
    m_line_height = (int)(App::instance().context()->character_height() * 1.2);
    m_rows = height() / m_line_height;
    m_columns = width() / App::instance().context()->character_width();
}

void Editor::render()
{
    box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
    m_line = 0;
    m_column = 0;
    buffer()->render();
}

void Editor::mark_current_line(int line)
{
    if (line < 0 || line >= rows())
        return;
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
    if (line < 0 || line >= rows() || column < 0 || column >= columns() || App::instance().modal() != nullptr)
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
    render_fixed(column_left(m_column), line_top(m_line), token.text, App::instance().color(token.color));
    m_column += (int)token.text.length();
}

void Editor::newline()
{
    ++m_line;
    m_column = 0;
}

bool Editor::dispatch(SDL_Keysym sym)
{
    if (Widget::dispatch(sym))
        return true;
    return buffer()->dispatch(sym);
}

void Editor::handle_mousedown(SDL_MouseButtonEvent const& event)
{
    auto offset_x = event.x - left();
    auto offset_y = event.y - top();
    auto column = offset_x / App::instance().context()->character_width();
    auto line = offset_y / line_height();
    m_mouse_down_at = { column, line };
    buffer()->mousedown(line, column);
}

void Editor::handle_motion(SDL_MouseMotionEvent const& event)
{
    if (m_mouse_down_at.has_value()) {
        auto offset_x = event.x - left();
        auto offset_y = event.y - top();
        auto column = offset_x / App::instance().context()->character_width();
        auto line = offset_y / line_height();
        if (column != m_mouse_down_at->left() || line != m_mouse_down_at->top()) {
            buffer()->motion(line, column);
        }
    }
}

void Editor::handle_click(SDL_MouseButtonEvent const& event)
{
    auto offset_x = event.x - left();
    auto offset_y = event.y - top();
    auto column = offset_x / App::instance().context()->character_width();
    auto line = offset_y / line_height();
    buffer()->click(line, column, event.clicks);
    m_mouse_down_at = {};
}

void Editor::handle_wheel(SDL_MouseWheelEvent const& event)
{
    buffer()->wheel(-event.y);
}

void Editor::handle_text_input()
{
    buffer()->handle_text_input();
}

std::optional<ScheduledCommand> Editor::command(std::string const& name) const
{
    if (auto ret = Widget::command(name); ret.has_value())
        return ret;
    if (auto ret = buffer()->command(name); ret.has_value())
        return ret;
    return {};
}

std::vector<ScheduledCommand> Editor::commands() const
{
    auto ret = Widget::commands();
    for (auto cmd : buffer()->commands())
        ret.push_back(cmd);
    return ret;
}

void Editor::new_file()
{
    m_buffers.emplace_back(new Document(this));
    m_current_buffer = m_buffers.back().get();
    m_current_buffer->on_activate();
}

std::string Editor::open_file(fs::path const& path)
{
    auto* doc = document();
    if (doc == nullptr || !doc->path().empty() || !doc->empty()) {
        new_file();
    }
    return document()->load(path);
}

std::string Editor::save_all() const
{
    for (auto* doc : documents()) {
        if (doc->path().empty())
            /* App::instance().schedule("query-filename-and-save") */;
        else
            doc->save();
    }
    return "";
}

Buffer* Editor::buffer() const
{
    return m_current_buffer;
}

Document* Editor::document() const
{
    return dynamic_cast<Document*>(m_current_buffer);
}

void Editor::switch_to(std::string const& buffer_name)
{
    auto* buf = buffer(buffer_name);
    if (buf != nullptr) {
        if (m_current_buffer != nullptr && buf != m_current_buffer)
            m_current_buffer->on_deactivate();
        m_current_buffer = buf;
        m_current_buffer->on_activate();
    }
}

std::vector<Buffer*> Editor::buffers() const
{
    std::vector<Buffer*> ret;
    ret.resize(m_buffers.size());
    for (auto& doc : m_buffers)
        ret.push_back(doc.get());
    std::sort(ret.begin(), ret.end(),
        [](auto const* p1, auto const* p2) {
            return p1->title() < p2->title();
        });
    return ret;
}

std::vector<Document*> Editor::documents() const
{
    std::vector<Document*> ret;
    ret.resize(m_buffers.size());
    for (auto& buf : m_buffers) {
        auto* doc = dynamic_cast<Document*>(buf.get());
        if (doc != nullptr)
            ret.push_back(doc);
    }
    std::sort(ret.begin(), ret.end(),
        [](auto const* p1, auto const* p2) {
            return p1->title() < p2->title();
        });
    return ret;
}

Document* Editor::document(fs::path const& path) const
{
    fs::path abs = fs::absolute(path);
    for (auto const& buffer : m_buffers) {
        auto doc = dynamic_cast<Document*>(buffer.get());
        if ((doc != nullptr) && (abs == doc->path()))
            return doc;
    }
    return nullptr;
}

Buffer* Editor::buffer(std::string const& title) const
{
    for (auto const& buffer : m_buffers) {
        if (title == buffer->title())
            return buffer.get();
    }
    return nullptr;
}

}
