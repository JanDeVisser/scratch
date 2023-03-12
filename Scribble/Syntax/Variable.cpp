/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Syntax/Variable.h>

namespace Scratch::Scribble {

extern_logging_category(scribble);

// -- VariableDeclaration ---------------------------------------------------

VariableDeclaration::VariableDeclaration(Span location, std::shared_ptr<Identifier> identifier, std::shared_ptr<Expression> expr, bool constant)
    : Statement(std::move(location))
    , m_identifier(std::move(identifier))
    , m_const(constant)
    , m_expression(std::move(expr))
{
}

std::string VariableDeclaration::attributes() const
{
    return format(R"(name="{}" is_const="{}")", name(), is_const());
}

Nodes VariableDeclaration::children() const
{
    if (m_expression)
        return { m_expression };
    return {};
}

std::string VariableDeclaration::to_string() const
{
    auto ret = format("{} {}", (is_const()) ? "const" : "var", name());
    if (m_expression)
        ret += format(" = {}", m_expression->to_string());
    return ret;
}

std::shared_ptr<Identifier> const& VariableDeclaration::identifier() const
{
    return m_identifier;
}

std::string const& VariableDeclaration::name() const
{
    return m_identifier->name();
}

bool VariableDeclaration::is_const() const
{
    return m_const;
}

std::shared_ptr<Expression> const& VariableDeclaration::expression() const
{
    return m_expression;
}

bool VariableDeclaration::is_complete() const
{
    return m_identifier != nullptr && m_identifier->is_complete() && (m_expression == nullptr || m_expression->is_complete());
}

}
