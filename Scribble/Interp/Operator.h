/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string_view>

#include <core/Logging.h>

namespace Scratch::Interp {

using namespace std::literals;
using namespace Obelix;

// Note: These are in order of highest-to-lowest operator precedence.
#define __enum_BinaryOperator(S) \
    S(Concatenate, "||")         \
    S(Multiplication, "*")       \
    S(Division, "/")             \
    S(Modulo, "%")               \
    S(Plus, "+")                 \
    S(Minus, "-")                \
    S(ShiftLeft, "<<")           \
    S(ShiftRight, ">>")          \
    S(BitwiseAnd, "&")           \
    S(BitwiseOr, "|")            \
    S(LessThan, "<")             \
    S(LessThanEquals, "<=")      \
    S(GreaterThan, ">")          \
    S(GreaterThanEquals, ">=")   \
    S(Equals, "=")               \
    S(NotEquals, "!=")           \
    S(And, "and")                \
    S(Or, "or")

enum class BinaryOperator {
#undef __BinaryOperator
#define __BinaryOperator(code, name) code,
    __enum_BinaryOperator(__BinaryOperator)
#undef __BinaryOperator
};

constexpr std::string_view BinaryOperator_name(BinaryOperator op)
{
    switch (op) {
#undef __BinaryOperator
#define __BinaryOperator(code, name) \
    case BinaryOperator::code:       \
        return name##sv;
        __enum_BinaryOperator(__BinaryOperator)
#undef __BinaryOperator
            default : fatal("Unreachable");
    }
}

#define __enum_UnaryOperator(S) \
    S(Minus, "-")               \
    S(Plus, "+")                \
    S(BitwiseNot, "~")          \
    S(Not, "!")

enum class UnaryOperator {
#undef __UnaryOperator
#define __UnaryOperator(code, name) code,
    __enum_UnaryOperator(__UnaryOperator)
#undef __UnaryOperator
};

constexpr std::string_view UnaryOperator_name(UnaryOperator op)
{
    switch (op) {
#undef __UnaryOperator
#define __UnaryOperator(code, name) \
    case UnaryOperator::code:       \
        return name##sv;
        __enum_UnaryOperator(__UnaryOperator)
#undef __UnaryOperator
            default : fatal("Unreachable");
    }
}

}
