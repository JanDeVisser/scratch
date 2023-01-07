/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Document.h>
#include <EditorState.h>
//#include <StatusBar.h>
#include <Widget.h>

namespace Scratch {

class Editor : public WindowedWidget {
public:
    Editor();

    [[nodiscard]] Document& document() { return m_current_document; }
    std::string open_file(std::string const& file_name);
    std::string save_file();
    std::string status();

    void render() override;
//    [[nodiscard]] bool handle(KeyCode) override;
    void append(DisplayToken const&);
    void newline();
private:
    std::vector<Document> m_documents { {} };
    Document& m_current_document { m_documents.front() };
    int m_line;
    int m_column;
};

}
