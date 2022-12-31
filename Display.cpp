/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <csignal>
#include <sys/ioctl.h>
#include <unistd.h>

#include <core/Error.h>
#include <core/Format.h>
#include <core/Logging.h>

#include <Display.h>

namespace Scratch {

extern_logging_category(scratch);

KeyMap keymap[] = {
#undef ENUM_KEYCODE
#define ENUM_KEYCODE(key, code, seq) \
{ key, seq },
ENUMERATE_KEYCODES(ENUM_KEYCODE)
#undef ENUM_KEYCODE
};

#define START_STYLE_SEQ(code) "\033[" #code "m"

inline void write_number(uint16_t num)
{
    char buffer[8];
    char *ptr;
    size_t bytes {0};
    if (num == 0) {
        buffer[0] = '0';
        ptr = buffer;
        bytes = 1;
    } else {
        ptr = buffer + 7;
        while (num > 0) {
            --ptr;
            bytes++;
            auto digit = num % 10;
            *ptr = (char) ('0' + digit);
            num /= 10;
        }
    }
    fwrite(ptr, 1, bytes, stdout);
}

inline void start_style(DisplayStyle style = DisplayStyle::None, std::pair<ForegroundColor, BackgroundColor> const& colors = { ForegroundColor::White, BackgroundColor::Black })
{
    switch (style) {
    case DisplayStyle::None:
        return;
    case DisplayStyle::Bold:
        fwrite(START_STYLE_SEQ(1), 1, 4, stdout);
        break;
    case DisplayStyle::Dim:
        fwrite(START_STYLE_SEQ(2), 1, 4, stdout);
        break;
    case DisplayStyle::Italic:
        fwrite(START_STYLE_SEQ(3), 1, 4, stdout);
        break;
    case DisplayStyle::Underline:
        fwrite(START_STYLE_SEQ(4), 1, 4, stdout);
        break;
    case DisplayStyle::Reverse:
        fwrite(START_STYLE_SEQ(7), 1, 4, stdout);
        break;
    case DisplayStyle::Colored:
        fwrite("\033[", 1, 2, stdout);
        write_number(static_cast<uint16_t>(colors.first));
        if (colors.second != BackgroundColor::None) {
            fwrite(";", 1, 1, stdout);
            write_number(static_cast<uint16_t>(colors.second));
        }
        fwrite("m", 1, 1, stdout);
        break;
    }
}

inline void reset_style()
{
    fwrite("\033[0m", 1, 4, stdout);
}

DisplayToken::DisplayToken(std::string text, DisplayStyle style, std::pair<ForegroundColor, BackgroundColor> colors)
    : m_text(std::move(text))
    , m_style(style)
    , m_colors(std::move(colors))
{
}

size_t DisplayToken::text_length() const
{
    return m_text.length();
}

void DisplayToken::render(size_t start, size_t end) const
{
    std::string text;
    if (start == 0 && end == 0)
        text = m_text;
    else
        text = m_text.substr(start, m_text.length() - start - end);
    switch (m_style) {
    case DisplayStyle::None:
        fwrite(text.c_str(), 1, text.length(), stdout);
        break;
    case DisplayStyle::Bold:
    case DisplayStyle::Dim:
    case DisplayStyle::Reverse:
    case DisplayStyle::Underline:
    case DisplayStyle::Italic:
    case DisplayStyle::Colored:
        start_style(m_style, m_colors);
        fwrite(text.c_str(), 1, text.length(), stdout);
        reset_style();
        break;
    }
}

DisplayLine::DisplayLine(DisplayToken display_token)
    : m_tokens({ std::move(display_token) })
{
}

void DisplayLine::append(DisplayToken display_token)
{
    m_tokens.push_back(std::move(display_token));
}

size_t DisplayLine::render() const
{
    size_t count = 0u;
    for (auto const& display_token : m_tokens) {
        display_token.render(0, 0);
        count += display_token.text_length();
    }
    return count;
}

void window_resize_signal(int)
{
}

ErrorOr<Display*, std::string> Display::create()
{
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
        return std::string("Please run the editor in the terminal");

    struct termios terminal_state {
        0
    };
    if (tcgetattr(STDIN_FILENO, &terminal_state) < 0)
        return format("Could not get the state of the terminal: {}", strerror(errno));

    struct termios new_term;
    memcpy(&new_term, &terminal_state, sizeof(struct termios));
    new_term.c_lflag &= ~ECHO;
    new_term.c_lflag &= ~ICANON;
    new_term.c_lflag &= ~ISIG;
    new_term.c_iflag &= ~IXON;
    if (tcsetattr(0, 0, &new_term)) {
        auto ret = format("Could not set the state of the terminal: {}", strerror(errno));
        tcsetattr(0, 0, &terminal_state);
        return ret;
    }

    struct sigaction act;
    struct sigaction old;
    act.sa_handler = window_resize_signal;
    if (sigaction(SIGWINCH, &act, &old) < 0) {
        auto ret = format("Could not set up window resize signal: {}", strerror(errno));
        sigaction(SIGWINCH, &old, nullptr);
        tcsetattr(0, 0, &terminal_state);
        return ret;
    }

    struct winsize w = { 0 };
    auto err = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (err != 0)
        return format("Could not get the size of the terminal: {}", strerror(errno));
    return new Display(w.ws_row, w.ws_col, terminal_state);
}

Display::Display(size_t rows, size_t columns, struct termios const& terminal_state)
    : m_rows(rows)
    , m_columns(columns)
    , m_terminal_state(terminal_state)
{
}

Display::~Display()
{
    debug(scratch, "~Display");
    tcsetattr(0, 0, &m_terminal_state);
}

void Display::append(DisplayToken display_token)
{
    if (m_lines.empty())
        m_lines.emplace_back(std::move(display_token));
    else
        m_lines.back().append(std::move(display_token));
}

void Display::newline()
{
    if (m_lines.empty())
        m_lines.emplace_back();
    if (m_lines.size() < rows())
        m_lines.emplace_back();
}

void Display::clear()
{
    m_lines.clear();
    m_cursor_row = m_cursor_column = 0;
}

void Display::render()
{
    fwrite("\033[H", 1, 3, stdout);
    for (auto const& line : m_lines) {
        auto count = line.render();
        while (count < columns()) {
            fwrite(" ", 1, 1, stdout);
            count++;
        }
    }
    fwrite("\033[J\033[", 1, 5, stdout);
    write_number(m_cursor_row+1);
    fwrite(";", 1, 1, stdout);
    write_number(m_cursor_column+1);
    fwrite("H", 1, 1, stdout);
    fflush(stdout);
}

void Display::cursor_at(size_t row, size_t column)
{
    m_cursor_row = row;
    m_cursor_column = column;
}

ErrorOr<KeyCode, int> Display::getkey()
{
    KeyCode ret = 0;
    do {
        char seq[MAX_ESC_SEQ_LEN] = { 0 };
        auto seq_len = ::read(STDIN_FILENO, seq, sizeof(seq));
        if (seq_len < 0) {
            if (errno == EINTR)
                continue;
            else
                return static_cast<int>(errno);
        }

        auto handle_single_char_sequence = [](unsigned char ch) -> KeyCode {
            if (ch == 0x0d || ch == 0x0a)
                return KEY_ENTER;
            if (0 < ch && ch < 0x1b)
                return ControlFlag | (ch + 'A' - 1);
            if (ch == 0x1b)
                return KEY_ESCAPE;
            if (ch == 0x7f)
                return KEY_BACKSPACE;
            if (isprint(ch))
                return static_cast<KeyCode>(ch);
            return 0;
        };

        std::string s;
        for (auto ch : seq) {
            if (ch == 0)
                break;
            s += format("0x{x} ", static_cast<int>(ch));
        }
        debug(scratch, "Read key sequence {}", s);

        if (seq_len == 1) {
            ret = handle_single_char_sequence(seq[0]);
            break;
        }
        for (auto const& key : keymap) {
            if (memcmp(seq, key.sequence, seq_len) == 0) {
                ret = key.code;
                break;
            }
        }
        if (ret != 0)
            break;
        for (auto ch : seq) {
            ret = handle_single_char_sequence(ch);
            if (ret != 0)
                break;
        }
        if (ret == 0) {
        }
    } while (ret == 0);
    debug(scratch, "getkey() returns {}", ret);
    return ret;
}

}
