/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Interp/ExpressionResult.h>

namespace Scratch::Interp {

using namespace Obelix;

ExpressionResult::ExpressionResult(Span location, Value value)
    : SyntaxNode(location)
    , m_value(std::move(value))
{
}

std::string ExpressionResult::to_string() const
{
    return m_value.to_string();
}

std::string ExpressionResult::attributes() const
{
    return format(R"(value="{}" type="{}")", m_value.to_string(), m_value.type_name());
}

Value const& ExpressionResult::value() const
{
    return m_value;
}

}
