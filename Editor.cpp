/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <cctype>

#include <App.h>
#include <Editor.h>
#include <SDLContext.h>

namespace Scratch {

extern_logging_category(scratch);

Editor::Editor()
    : WindowedWidget(Vector<int,4> { 0, 0, 0, 0 }, Vector<int,4> { WIDGET_BORDER_X, WIDGET_BORDER_X, WIDGET_BORDER_Y, WIDGET_BORDER_Y })
{
}

int Editor::rows() const
{
    return content_height() / (int)(App::instance().context()->character_height() * 1.2);
}

int Editor::columns() const
{
    return content_width() / App::instance().context()->character_width();
}

void Editor::render()
{
    m_line = 0;
    m_column = 0;
    document().render(this);
}

void Editor::append(DisplayToken const& token)
{
    auto x = m_column * App::instance().context()->character_width();
    auto y = m_line * App::instance().context()->character_height() * 1.2;
    render_text(x, (int) y, token.text, App::instance().color(token.color));
    m_column += (int) token.text.length();
}

void Editor::newline()
{
    ++m_line;
    m_column = 0;
}

//bool Editor::handle(KeyCode code)
//{
//    return document().handle(code);
//}

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


std::string Editor::status()
{
    char buffer[81];
    snprintf(buffer, 80, "%-20.20s %4d : %4d", document().filename().c_str(), document().point_line(), document().point_column());
    return { buffer };
}

}
