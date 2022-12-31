/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <termios.h>
#include <vector>

#include <core/Error.h>
#include <core/Format.h>

using namespace Obelix;

#define MAX_ESC_SEQ_LEN 32

namespace Scratch {

using KeySequence=char const*;
constexpr static uint16_t ControlFlag = 0x0100;
constexpr static uint16_t FunctionFlag = 0x0200;

#define ENUMERATE_KEYCODES(S)                              \
    S(KEY_ESCAPE, 0x1b, "\x1b")                            \
    S(KEY_ENTER, 0x0d, "\x0d")                             \
    S(KEY_BACKSPACE, FunctionFlag | 0x08, "\x7f")          \
    S(KEY_DELETE, FunctionFlag | 0x7f, "\x1b\x5b\x33\x7e") \
    S(KEY_LEFT, FunctionFlag | 'h', "\x1b\x5b\x44")        \
    S(KEY_DOWN, FunctionFlag | 'j', "\x1b\x5b\x42")        \
    S(KEY_RIGHT, FunctionFlag | 'l', "\x1b\x5b\x43")       \
    S(KEY_UP, FunctionFlag | 'k', "\x1b\x5b\x41")          \
    S(KEY_F1, ControlFlag | '1', "\x1b\x4f\x50")           \
    S(KEY_F2, ControlFlag | '2', "\x1b\x4f\x51")           \
    S(KEY_F3, ControlFlag | '3', "\x1b\x4f\x52")           \
    S(KEY_F4, ControlFlag | '4', "\x1b\x4f\x53")           \
    S(KEY_F5, ControlFlag | '5', "\x1b\x5b\x31\x35\x7e")   \
    S(KEY_F6, ControlFlag | '6', "\x1b\x5b\x31\x37\x7e")   \
    S(KEY_F7, ControlFlag | '7', "\x1b\x5b\x31\x38\x7e")   \
    S(KEY_F8, ControlFlag | '8', "\x1b\x5b\x31\x39\x7e")   \
    S(KEY_F9, ControlFlag | '9', "\x1b\x5b\x32\x30\x7e")   \
    S(KEY_F10, ControlFlag | 'A', "\x1b\x5b\x32\x31\x7e")  \
    S(KEY_F11, ControlFlag | 'B', "\x1b\x5b\x32\x33\x7e")  \
    S(KEY_F12, ControlFlag | 'C', "\x1b\x5b\x32\x34\x7e")

struct KeyCode {
    constexpr KeyCode(int c, uint16_t flags = 0)
        : code(c | flags)
    {
    }

    uint16_t code;
    constexpr operator int () const { return code; }
    constexpr bool operator==(KeyCode other) const { return code == other.code; }
    constexpr bool operator==(int other) const { return code == other; }

    [[nodiscard]] bool is_ctrl() const
    {
        return code & ControlFlag;
    }

    [[nodiscard]] bool is_function() const
    {
        return code & FunctionFlag;
    }

    [[nodiscard]] std::string to_string() const
    {
        std::string ret;
        if (isprint(code)) {
            ret += static_cast<char>(code);
            return ret;
        }

        if (is_ctrl()) {
            return format("^{c}", static_cast<char>(code & 0xFFFF));
        }

        switch (code) {
#undef ENUM_KEYCODE
#define ENUM_KEYCODE(key, c, seq) \
        case c: return #key;
            ENUMERATE_KEYCODES(ENUM_KEYCODE)
#undef ENUM_KEYCODE
        default:
            return format("{}", code);
        }
    }

};

[[maybe_unused]] static KeyCode ctrl(KeyCode key) {
    assert((key.code >= 'a' && key.code <= 'z') || (key.code >= 'A' && key.code <= 'Z'));
    return { toupper(key.code), ControlFlag };
}

[[maybe_unused]] static KeyCode key_f(int f) {
    if (0 < f && f < 10)
        return { ('0' + f), FunctionFlag };
    if (10 <= f && f <= 12)
        return { ('A' + f), FunctionFlag };
    return { 0 };
}

#undef ENUM_KEYCODE
#define ENUM_KEYCODE(key, code, seq) \
    constexpr static KeyCode key(code);
ENUMERATE_KEYCODES(ENUM_KEYCODE)
#undef ENUM_KEYCODE

constexpr static KeyCode CTRL_A('A', ControlFlag);
constexpr static KeyCode CTRL_B('B', ControlFlag);
constexpr static KeyCode CTRL_C('C', ControlFlag);
constexpr static KeyCode CTRL_D('D', ControlFlag);
constexpr static KeyCode CTRL_E('E', ControlFlag);
constexpr static KeyCode CTRL_F('F', ControlFlag);
constexpr static KeyCode CTRL_G('G', ControlFlag);
constexpr static KeyCode CTRL_H('H', ControlFlag);
constexpr static KeyCode CTRL_I('U', ControlFlag);
constexpr static KeyCode CTRL_J('J', ControlFlag);
constexpr static KeyCode CTRL_K('K', ControlFlag);
constexpr static KeyCode CTRL_L('L', ControlFlag);
constexpr static KeyCode CTRL_M('M', ControlFlag);
constexpr static KeyCode CTRL_N('N', ControlFlag);
constexpr static KeyCode CTRL_O('O', ControlFlag);
constexpr static KeyCode CTRL_P('P', ControlFlag);
constexpr static KeyCode CTRL_Q('Q', ControlFlag);
constexpr static KeyCode CTRL_R('R', ControlFlag);
constexpr static KeyCode CTRL_S('S', ControlFlag);
constexpr static KeyCode CTRL_T('T', ControlFlag);
constexpr static KeyCode CTRL_U('U', ControlFlag);
constexpr static KeyCode CTRL_V('V', ControlFlag);
constexpr static KeyCode CTRL_W('W', ControlFlag);
constexpr static KeyCode CTRL_X('X', ControlFlag);
constexpr static KeyCode CTRL_Y('Y', ControlFlag);
constexpr static KeyCode CTRL_Z('Z', ControlFlag);

struct KeyMap {
    KeyCode code;
    KeySequence sequence;
};

extern KeyMap keymap[];

enum class DisplayStyle {
    None,
    Colored,
    Bold,
    Dim,
    Underline,
    Reverse,
    Italic,
};

enum class ForegroundColor {
    None = 0,
    Black = 30,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
    Gray = 90,
    BrightRed,
    BrightGreen,
    BrightYellow,
    BrightBlue,
    BrightMagenta,
    BrightWhite,
};

enum class BackgroundColor {
    None = 0,
    Black = 40,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
    Gray = 100,
    BrightRed,
    BrightGreen,
    BrightYellow,
    BrightBlue,
    BrightMagenta,
    BrightWhite,
};

class DisplayToken {
public:
    DisplayToken(std::string, DisplayStyle = DisplayStyle::None,
            std::pair<ForegroundColor, BackgroundColor> = { ForegroundColor::White, BackgroundColor::Black });
    void render(size_t, size_t) const;
    [[nodiscard]] size_t text_length() const;

private:
    std::string m_text {};
    DisplayStyle m_style;
    std::pair<ForegroundColor, BackgroundColor> m_colors {};
};

class DisplayLine {
public:
    DisplayLine() = default;
    DisplayLine(DisplayToken);
    void append(DisplayToken);
    size_t render() const;
private:
    std::vector<DisplayToken> m_tokens;
};

class Display {
public:
    static ErrorOr<Display*, std::string> create();
    ~Display();
    void append(DisplayToken);
    void newline();
    void clear();
    void render();
    void cursor_at(size_t, size_t);
    [[nodiscard]] size_t rows() const { return m_rows; }
    [[nodiscard]] size_t columns() const { return m_columns; }
    [[nodiscard]] static ErrorOr<KeyCode, int> getkey();

private:
    Display(size_t, size_t, struct termios const&);

    size_t m_rows;
    size_t m_columns;
    size_t m_cursor_row { 0 };
    size_t m_cursor_column { 0 };
    std::vector<DisplayLine> m_lines;
    struct termios m_terminal_state;
};

}

namespace Obelix {

using namespace Scratch;

template<>
struct Converter<KeyCode> {
    static std::string to_string(KeyCode const& keycode)
    {
        return keycode.to_string();
    }

    static long to_long(KeyCode const& keycode)
    {
        return keycode.code;
    }

    static double to_double(KeyCode const& keycode)
    {
        return keycode.code;
    }
};

}
