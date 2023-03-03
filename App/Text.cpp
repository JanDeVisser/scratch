/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <cctype>

#include <App/Text.h>

namespace Scratch {

bool isPrintable(int cp)
{
    if (cp > 255)
        return false;

    return !!::isprint(cp);
}

int countUtf8BytesFromChar(unsigned int c)
{
    if (c < 0x80)
        return 1;
    if (c < 0x800)
        return 2;
    if (c >= 0xdc00 && c < 0xe000)
        return 0;
    if (c >= 0xd800 && c < 0xdc00)
        return 4;

    return 3;
}

int countUtf8BytesFromStr(CodePoint const* in_text, CodePoint const* in_text_end)
{
    int bytes_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text) {
        auto c = (unsigned int)(*in_text++);
        if (c < 0x80)
            bytes_count++;
        else
            bytes_count += countUtf8BytesFromChar(c);
    }

    return bytes_count;
}

int charFromUtf8(unsigned int* out_char, char const* in_text, char const* in_text_end)
{
    unsigned int c;
    auto* str = (unsigned char const*)in_text;
    if (!(*str & 0x80)) {
        *out_char = *str;
        return 1;
    }
    if ((*str & 0xe0) == 0xc0) {
        *out_char = 0xFFFD; // Will be invalid but not end of string.
        if (in_text_end && in_text_end - (char const*)str < 2)
            return 1;
        if (*str < 0xc2)
            return 2;
        str++;
        if ((*str & 0xc0) != 0x80)
            return 2;
        *out_char = *str & 0x3f;
        return 2;
    }
    if ((*str & 0xf0) == 0xe0) {
        *out_char = 0xFFFD; // Will be invalid but not end of string.
        if (in_text_end && in_text_end - (char const*)str < 3)
            return 1;
        if (*str == 0xe0 && (str[1] < 0xa0 || str[1] > 0xbf))
            return 3;
        if (*str == 0xed && str[1] > 0x9f)
            return 3; // str[1] < 0x80 is checked below.
        str++;
        if ((*str & 0xc0) != 0x80)
            return 3;
        str++;
        if ((*str & 0xc0) != 0x80)
            return 3;
        *out_char = *str & 0x3f;
        return 3;
    }
    if ((*str & 0xf8) == 0xf0) {
        *out_char = 0xFFFD; // Will be invalid but not end of string.
        if (in_text_end && in_text_end - (char const*)str < 4)
            return 1;
        if (*str > 0xf4)
            return 4;
        if (*str == 0xf0 && (str[1] < 0x90 || str[1] > 0xbf))
            return 4;
        if (*str == 0xf4 && str[1] > 0x8f)
            return 4; // str[1] < 0x80 is checked below.
        c = (unsigned int)((*str++ & 0x07) << 18);
        if ((*str & 0xc0) != 0x80)
            return 4;
        c += (unsigned int)((*str++ & 0x3f) << 12);
        if ((*str & 0xc0) != 0x80)
            return 4;
        c += (unsigned int)((*str++ & 0x3f) << 6);
        if ((*str & 0xc0) != 0x80)
            return 4;
        c += (*str & 0x3f);
        // UTF-8 encodings of values used in surrogate pairs are invalid.
        if ((c & 0xFFFFF800) == 0xD800)
            return 4;
        *out_char = c;
        return 4;
    }
    *out_char = 0;

    return 0;
}

int charToUtf8(char* buf, int buf_size, unsigned int c)
{
    if (c < 0x80) {
        buf[0] = (char)c;

        return 1;
    }
    if (c < 0x800) {
        if (buf_size < 2)
            return 0;
        buf[0] = (char)(0xc0 + (c >> 6));
        buf[1] = (char)(0x80 + (c & 0x3f));

        return 2;
    }
    if (c >= 0xdc00 && c < 0xe000) {
        return 0;
    }
    if (c >= 0xd800 && c < 0xdc00) {
        if (buf_size < 4)
            return 0;
        buf[0] = (char)(0xf0 + (c >> 18));
        buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((c)&0x3f));

        return 4;
    }
    // else if (c < 0x10000)
    {
        if (buf_size < 3)
            return 0;
        buf[0] = (char)(0xe0 + (c >> 12));
        buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[2] = (char)(0x80 + ((c)&0x3f));

        return 3;
    }
}

int strFromUtf8(CodePoint* buf, int buf_size, char const* in_text, char const* in_text_end, char const** in_text_remaining) {
    CodePoint* buf_out = buf;
    CodePoint* buf_end = buf + buf_size;
    while (buf_out < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text) {
        unsigned int c;
        in_text += charFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        if (c < 0x10000) // FIXME: Losing characters that don't fit in 2 bytes.
            *buf_out++ = (CodePoint)c;
    }
    *buf_out = 0;
    if (in_text_remaining)
        *in_text_remaining = in_text;

    return (int)(buf_out - buf);
}

int strToUtf8(char* buf, int buf_size, CodePoint const* in_text, CodePoint const* in_text_end)
{
    char* buf_out = buf;
    char const* buf_end = buf + buf_size;
    while (buf_out < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text) {
        auto c = (unsigned int)(*in_text++);
        if (c < 0x80)
            *buf_out++ = (char)c;
        else
            buf_out += charToUtf8(buf_out, (int)(buf_end - buf_out - 1), c);
    }
    *buf_out = 0;

    return (int)(buf_out - buf);
}

std::string strToUtf8StdStr(CodePoint const* in_text, CodePoint const* in_text_end)
{
    int sz = countUtf8BytesFromStr(in_text, in_text_end);
    std::string result;
    result.resize(sz + 1);
    strToUtf8(&(*result.begin()), (int)result.length(), in_text, in_text_end);

    return result;
}

int expectUtf8Char(char const* ch)
{
#define TAKE(__ch, __c, __r) \
    do {                      \
        (__c) = *(__ch)++;    \
        (__r)++;              \
    } while (0)
#define COPY(__ch, __c, __r, __cp)                            \
    do {                                                       \
        TAKE((__ch), (__c), (__r));                           \
        (__cp) = ((__cp) << 6) | ((unsigned char)(__c)&0x3fu); \
    } while (0)
#define TRANS(__m, __cp, __g)                              \
    do {                                                    \
        (__cp) &= (((__g)[(unsigned char)c] & (__m)) != 0); \
    } while (0)
#define TAIL(__ch, __c, __r, __cp, __g)     \
    do {                                     \
        COPY((__ch), (__c), (__r), (__cp)); \
        TRANS(0x70, (__cp), (__g));         \
    } while (0)

    static const unsigned char range[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
    };

    int result = 0;
    unsigned codepoint;
    unsigned char type;
    char c;

    if (!ch)
        return 0;

    TAKE(ch, c, result);
    if (!(c & 0x80)) {
        return 1;
    }

    type = range[(unsigned char)c];
    codepoint = (0xff >> type) & (unsigned char)c;

    switch (type) {
    case 2:
        TAIL(ch, c, result, codepoint, range);
        return result;
    case 3:
        TAIL(ch, c, result, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        return result;
    case 4:
        COPY(ch, c, result, codepoint);
        TRANS(0x50, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        return result;
    case 5:
        COPY(ch, c, result, codepoint);
        TRANS(0x10, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        return result;
    case 6:
        TAIL(ch, c, result, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        return result;
    case 10:
        COPY(ch, c, result, codepoint);
        TRANS(0x20, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        return result;
    case 11:
        COPY(ch, c, result, codepoint);
        TRANS(0x60, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        TAIL(ch, c, result, codepoint, range);
        return result;
    default:
        return 0;
    }

#undef TAKE
#undef COPY
#undef TRANS
#undef TAIL
}

Char takeUtf8Bytes(char const* str, int n)
{
    union {
        Char ui {0};
        char ch[4];
    } u;
    if (n > 4) n = 4;
    for (int i = 0; i < n; ++i)
        u.ch[i] = str[i];
    for (int i = n; i < 4; ++i)
        u.ch[i] = '\0';

    return u.ui;
}

int countUtf8Bytes(Char chr)
{
    int ret = 0;
    union {
        Char ui {0};
        char ch[4];
    } u;
    u.ui = chr;
    for (int i = 0; i < 4; ++i) {
        if (u.ch[i])
            ret = i + 1;
        else
            break;
    }

    return ret;
}

int appendUtf8ToStdStr(std::string& buf, Char chr)
{
    int ret = 0;
    union {
        Char ui { 0 };
        char ch[4];
    } u;
    u.ui = chr;
    for (int i = 0; i < 4; ++i) {
        if (u.ch[i]) {
            buf.push_back(u.ch[i]);
            ret = i + 1;
        } else {
            break;
        }
    }
    return ret;
}

}
