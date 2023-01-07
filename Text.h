/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

namespace Scratch {

using CodePoint=unsigned short;
using Char=unsigned int;

bool isPrintable(int);
int countUtf8BytesFromChar(unsigned int);
int countUtf8BytesFromStr(CodePoint const* in_text, CodePoint const* in_text_end);
int charFromUtf8(unsigned int* out_char, char const* in_text, char const* in_text_end);
int charToUtf8(char* buf, int buf_size, unsigned int c);
int strFromUtf8(CodePoint* buf, int buf_size, char const* in_text, char const* in_text_end, char const** in_text_remaining = nullptr);
int strToUtf8(char* buf, int buf_size, CodePoint const* in_text, CodePoint const* in_text_end);
std::string strToUtf8StdStr(CodePoint const* in_text, CodePoint const* in_text_end);
int expectUtf8Char(char const* ch);
Char takeUtf8Bytes(char const* str, int n);
int countUtf8Bytes(Char chr);
int appendUtf8ToStdStr(std::string& buf, Char chr);

}
