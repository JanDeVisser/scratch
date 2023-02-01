/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App.h>
#include <EditorState.h>

namespace Scratch {

Glyph::Glyph(Char ch, PaletteIndex idx)
    : character(ch)
    , colorIndex(idx)
    , multiLineComment(false)
{
    if (ch <= 255) {
        codepoint = (CodePoint)ch;
    } else {
        char const* txt = (char const*)(&ch);
        char const* tend = txt + sizeof(Char);
        unsigned int cp = 0;
        charFromUtf8(&cp, txt, tend);
        codepoint = (CodePoint)cp;
    }
}

Coordinates::Coordinates(int ln, int col)
    : line(ln)
    , column(col)
{
    assert(ln >= 0);
    assert(col >= 0);
}

Coordinates Coordinates::invalid()
{
    static Coordinates invalid(-1, -1);
    return invalid;
}

auto Coordinates::operator<=>(Coordinates const& o) const
{
    if (line != o.line)
        return line - o.line;
    return column - o.column;
}

#if 0

bool UndoRecord::similar(UndoRecord const* o) const
{
    if (o == nullptr)
        return false;
    if (type != o->type)
        return false;
    if (start.line != o->start.line || end.line != o->end.line)
        return false;

    auto isalpha = [](char ch) {
        return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
    };
    auto isnum = [](char ch) {
        return ch >= '0' && ch <= '9';
    };
    auto isblank = [](char ch) {
        return static_cast<bool>(isspace(ch));
    };
    if ((content.length() == 1 && isalpha(*content.begin())) && (o->content.length() == 1 && isalpha(*o->content.begin()))) {
        return true;
    }
    if ((content.length() == 1 && isnum(*content.begin())) && (o->content.length() == 1 && isnum(*o->content.begin()))) {
        return true;
    }
    if ((content.length() == 1 && isblank(*content.begin())) && (o->content.length() == 1 && isblank(*o->content.begin()))) {
        return true;
    }
    if (content.length() > 1 && content.length() <= 4 && o->content.length() > 1 && o->content.length() <= 4) {
        int l = expectUtf8Char(content.c_str());
        int r = expectUtf8Char(o->content.c_str());
        if ((int)content.length() == l && (int)o->content.length() == r)
            return true;
    }

    return false;
}

void UndoRecord::undo(App* app)
{
    if (!content.empty()) {
        switch (type) {
        case UndoType::Add: {
            app->state(after);
            app->removeRange(start, end);
            if (!overwritten.empty()) {
                Coordinates st = start;
                app->insertTextAt(st, overwritten.c_str());
            }
            app->onChanged(start, start, -1);
        } break;
        case UndoType::Remove: {
            Coordinates st = start;
            app->insertTextAt(st, content.c_str());
            app->onChanged(st, end, -1);
        } break;
        case UndoType::Indent: {
            assert(end.line - start.line + 1 == (int)content.length());

            for (int i = start.line; i <= end.line; ++i) {
                Line& line = app->m_code_lines[i];
                const char op = content[i - start.line];
                if (op == 0) {
                    // Does nothing.
                } else if (op == std::numeric_limits<char>::max()) {
                    const Glyph& g = *line.begin();
                    if (g.character == '\t') {
                        line.erase(line.begin());
                    } else {
                        assert(false);
                    }

                    Coordinates pos(i, 0);
                    app->onChanged(pos, pos, -1);
                } else {
                    assert(false);
                }
            }
        }

        break;
        case UndoType::Unindent: {
            assert(end.line - start.line + 1 == (int)content.length());

            for (int i = start.line; i <= end.line; ++i) {
                Line& line = app->_codeLines[i];
                char op = content[i - start.line];
                if (op == 0) {
                    // Does nothing.
                } else if (op == std::numeric_limits<char>::max()) {
                    line.insert(line.begin(), Glyph('\t', PaletteIndex::Default));

                    Coordinates pos(i, 0);
                    app->onChanged(pos, pos, -1);
                } else if (op > 0) {
                    while (op--) {
                        line.insert(line.begin(), Glyph(' ', PaletteIndex::Default));
                    }

                    Coordinates pos(i, 0);
                    app->onChanged(pos, pos, -1);
                } else {
                    assert(false);
                }
            }
        }

        break;
        }
    }

    app->_state = before;
    app->ensureCursorVisible();

    app->onModified();
}

void UndoRecord::redo(App* app)
{
    if (!content.empty()) {
        switch (type) {
        case UndoType::Add: {
            app->_state = before;

            app->removeSelection();

            Coordinates st = start;
            app->insertTextAt(st, content.c_str());
            app->colorize(st.line - 1, end.line - st.line + 1);

            app->onChanged(st, end, 1);
        }

        break;
        case UndoType::Remove: {
            app->removeRange(start, end);
            app->colorize(start.line - 1, end.line - start.line + 1);

            app->onChanged(start, start, 1);
        }

        break;
        case UndoType::Indent: {
            assert(end.line - start.line + 1 == (int)content.length());

            for (int i = start.line; i <= end.line; ++i) {
                Line& line = app->_codeLines[i];
                const char op = content[i - start.line];
                if (op == 0) {
                    // Does nothing.
                } else if (op == std::numeric_limits<char>::max()) {
                    line.insert(line.begin(), Glyph('\t', PaletteIndex::Default));

                    Coordinates pos(i, 0);
                    app->onChanged(pos, pos, 1);
                } else {
                    assert(false);
                }
            }
        }

        break;
        case UndoType::Unindent: {
            assert(end.line - start.line + 1 == (int)content.length());

            for (int i = start.line; i <= end.line; ++i) {
                Line& line = app->_codeLines[i];
                char op = content[i - start.line];
                if (op == 0) {
                    // Does nothing.
                } else if (op == std::numeric_limits<char>::max()) {
                    const Glyph& g = *line.begin();
                    if (g.character == '\t') {
                        line.erase(line.begin());
                    } else {
                        assert(false);
                    }

                    Coordinates pos(i, 0);
                    app->onChanged(pos, pos, 1);
                } else if (op > 0) {
                    while (op--) {
                        const Glyph& g = *line.begin();
                        if (g.character == ' ') {
                            line.erase(line.begin());
                        } else {
                            assert(false);
                        }
                    }

                    Coordinates pos(i, 0);
                    app->onChanged(pos, pos, 1);
                } else {
                    assert(false);
                }
            }
        }

        break;
        }
    }

    app->_state = after;
    app->ensureCursorVisible();

    app->onModified();
}

#endif

static unsigned s_ansicolors[16] = {
};

Palette const& DarkPalette()
{
    static Palette p = {
        0xffffffff, // None.
        0xffd69c56, // Keyword.
        0xffa8ceb5, // Number.
        0xff859dd6, // String.
        0xff70a0e0, // Char literal.
        0xffb4b4b4, // Punctuation.
        0xff409090, // Preprocessor.
        0xffdadada, // Identifier.
        0xffb0c94e, // Known identifier.
        0xffc040a0, // Preproc identifier.
        0xff4aa657, // Comment (single line).
        0xff4aa657, // Comment (multi line).
        0xff2C2C2C, // Background.
        0xffe0e0e0, // Cursor.
        0xffa06020, // Selection.
        0x804d00ff, // ErrorMarker.
        0x40f08000, // Breakpoint.
        0xffaf912b, // Line number.
        0x40000000, // Current line fill.
        0x40808080, // Current line fill (inactive).
        0x40a0a0a0, // Current line edge.
        0xff84f2ef, // Line edited.
        0xff307457, // Line edited saved.
        0xfffa955f, // Line edited reverted.
    };
    return p;
}

Palette const& LightPalette()
{
    static Palette p = {
        0xff000000, // None.
        0xffff0c06, // Keyword.
        0xff008000, // Number.
        0xff2020a0, // String.
        0xff304070, // Char literal.
        0xff000000, // Punctuation.
        0xff409090, // Preprocessor.
        0xff404040, // Identifier.
        0xff606010, // Known identifier.
        0xffc040a0, // Preproc identifier.
        0xff205020, // Comment (single line).
        0xff405020, // Comment (multi line).
        0xffffffff, // Background.
        0xff000000, // Cursor.
        0xff600000, // Selection.
        0xa00010ff, // ErrorMarker.
        0x80f08000, // Breakpoint.
        0xff505000, // Line number.
        0x40000000, // Current line fill.
        0x40808080, // Current line fill (inactive).
        0x40000000, // Current line edge.
        0xff84f2ef, // Line edited.
        0xff307457, // Line edited saved.
        0xfffa955f, // Line edited reverted.
    };
    return p;
}

Palette const& RetroBluePalette()
{
    static Palette p = {
        0xff00ffff, // None.
        0xffffff00, // Keyword.
        0xff00ff00, // Number.
        0xff808000, // String.
        0xff808000, // Char literal.
        0xffffffff, // Punctuation.
        0xff008000, // Preprocessor.
        0xff00ffff, // Identifier.
        0xffffffff, // Known identifier.
        0xffff00ff, // Preproc identifier.
        0xff808080, // Comment (single line).
        0xff404040, // Comment (multi line).
        0xff800000, // Background.
        0xff0080ff, // Cursor.
        0xffffff00, // Selection.
        0xa00000ff, // ErrorMarker.
        0x80ff8000, // Breakpoint.
        0xff808000, // Line number.
        0x40000000, // Current line fill.
        0x40808080, // Current line fill (inactive).
        0x40000000, // Current line edge.
        0xff84f2ef, // Line edited.
        0xff307457, // Line edited saved.
        0xfffa955f, // Line edited reverted.
    };

    return p;
}

}
