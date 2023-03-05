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

ExpressionResultList::ExpressionResultList(Span location, Values values)
    : SyntaxNode(location)
    , m_values(std::move(values))
{
}

std::string ExpressionResultList::to_string() const
{
    return Obelix::to_string<Values>()(m_values);
}

std::string ExpressionResultList::attributes() const
{
    return format(R"(ExpressionResultList attributes TODO)");
}

Values const& ExpressionResultList::values() const
{
    return m_values;
}

}
