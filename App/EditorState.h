/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <App/Text.h>

namespace Scratch {

enum class PaletteIndex : uint8_t {
    Default,
    Keyword,
    Number,
    String,
    CharLiteral,
    Punctuation,
    Preprocessor,
    Identifier,
    KnownIdentifier,
    PreprocIdentifier,
    Comment,
    MultiLineComment,
    Background,
    Cursor,
    Selection,
    ErrorMarker,
    Breakpoint,
    LineNumber,
    CurrentLineFill,
    CurrentLineFillInactive,
    CurrentLineEdge,
    LineEdited,
    LineEditedSaved,
    LineEditedReverted,
    ANSIBlack,
    ANSIRed,
    ANSIGreen,
    ANSIYellow,
    ANSIBlue,
    ANSIMagenta,
    ANSICyan,
    ANSIWhite,
    ANSIBrightBlack,
    ANSIBrightRed,
    ANSIBrightGreen,
    ANSIBrightYellow,
    ANSIBrightBlue,
    ANSIBrightMagenta,
    ANSIBrightCyan,
    ANSIBrightWhite,
    Max
};

using Palette = std::array<unsigned, (size_t)PaletteIndex::Max>;

struct DisplayToken {
    explicit DisplayToken(std::string_view t, PaletteIndex c = PaletteIndex::Default)
        : text(t)
        , color(c)
    {
    }

    explicit DisplayToken(std::string t, PaletteIndex c = PaletteIndex::Default)
        : text_string(std::move(t))
        , text(text_string.value())
        , color(c)
    {
    }

    std::optional<std::string> text_string {};
    std::string_view text;
    PaletteIndex color;
};

enum ShortcutType {
    UndoRedo = 1 << 0,
    CopyCutPaste = 1 << 2,
    All = UndoRedo | CopyCutPaste
};

enum class LineState : uint8_t {
    None,
    Edited,
    EditedSaved,
    EditedReverted
};

enum class UndoType : uint8_t {
    Add,
    Remove,
    Indent,
    Unindent
};

struct Breakpoint {
    int line = -1;
    bool enabled = false;
    std::string condition;
};

struct Glyph {
    CodePoint codepoint = 0;
    Char character = 0;
    PaletteIndex colorIndex : 7;
    bool multiLineComment : 1;

    Glyph(Char ch, PaletteIndex idx);
};

struct Coordinates {
    int line = 0, column = 0;

    Coordinates() = default;
    Coordinates(int ln, int col);
    static Coordinates invalid();
    auto operator<=>(Coordinates const& o) const;
};

struct EditorState {
    Coordinates selectionStart;
    Coordinates selectionEnd;
    Coordinates cursorPosition;
};

#if 0

struct UndoRecord {
    UndoRecord() = default;
    ~UndoRecord() = default;

    bool similar(UndoRecord const* o) const;

    void undo(class App*);
    void redo(class App*);

    UndoType type { UndoType::Add };

    std::string overwritten;
    std::string content;
    Coordinates start;
    Coordinates end;

    EditorState before;
    EditorState after;
};

using UndoBuffer = std::vector<UndoRecord>;

#endif

using KeyStates = std::vector<uint8_t>;
using InputBuffer = std::basic_string<CodePoint, std::char_traits<CodePoint>, std::allocator<CodePoint>>;

Palette const& DarkPalette();
Palette const& LightPalette();
Palette const& RetroBluePalette();

}
