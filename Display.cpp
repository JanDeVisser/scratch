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
    case DisplayStyle::Reverse:
        fwrite(START_STYLE_SEQ(7), 1, 4, stdout);
        break;
    case DisplayStyle::Underline:
        fwrite(START_STYLE_SEQ(4), 1, 4, stdout);
        break;
    case DisplayStyle::Colored:
        fprintf(stdout, "\033[%d;%dm", static_cast<int>(colors.first), static_cast<int>(colors.second));
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

void DisplayLine::render(size_t offset, size_t length)
{
    size_t count = 0u;
    auto right_margin = offset + length;
    for (auto const& display_token : m_tokens) {
        if (count + display_token.text_length() < offset) {
            count += display_token.text_length();
            continue;
        }
        if (count < offset && count + display_token.text_length() < offset) {
            display_token.render(offset - count, 0);
            count += display_token.text_length();
            continue;
        }
        if (count >= offset && count + display_token.text_length() < right_margin) {
            display_token.render(0, 0);
            count += display_token.text_length();
            continue;
        }
        if (count >= offset && count + display_token.text_length() >= right_margin) {
            display_token.render(0, right_margin - count - display_token.text_length());
            count += display_token.text_length();
            break;
        }
    }
    while (count < offset + length) {
        fwrite(" ", 1, 1, stdout);
        count++;
    }
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
    m_lines.emplace_back();
}

void Display::clear()
{
    m_lines.clear();
}

void Display::render(size_t cursor_row, size_t cursor_col)
{
    fwrite("\033[H", 1, 3, stdout);
    for (auto ix = m_top; ix < m_top + m_rows && ix < m_lines.size(); ++ix) {
        m_lines[ix].render(m_left, m_columns);
    }
    fwrite("\033[J", 1, 3, stdout);
    fprintf(stdout, "\033[%zu;%zuH", cursor_row + 1, cursor_col + 1);
    fflush(stdout);
}

extern_logging_category(scratch);

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
            if (0 < ch && ch < '\033')
                return ControlFlag | (ch + 'a' - 1);
            if (isprint(ch))
                return static_cast<KeyCode>(ch);
            return 0;
        };

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
    } while (ret == 0);
    debug(scratch, "getkey() returns {}", ret);
    return ret;
}

}
