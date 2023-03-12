/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Syntax/Expression.h>

namespace Scratch::Scribble {

extern_logging_category(scribble);

// -- Expression ------------------------------------------------------------

Expression::Expression(Span location)
    : SyntaxNode(location)
{
}

// -- ExpressionList --------------------------------------------------------

ExpressionList::ExpressionList(Span location, Expressions expressions)
    : Expression(location)
    , m_expressions(std::move(expressions))
{
}

Expressions const& ExpressionList::expressions() const
{
    return m_expressions;
}

Nodes ExpressionList::children() const
{
    Nodes ret;
    for (auto const& expr : expressions()) {
        ret.push_back(expr);
    }
    return ret;
}

std::string ExpressionList::to_string() const
{
    Strings ret;
    for (auto const& expr : expressions()) {
        ret.push_back(expr->to_string());
    }
    return join(ret, ", ");
}

bool ExpressionList::is_complete() const
{
    return std::all_of(m_expressions.begin(), m_expressions.end(), [](auto const& expr) -> bool {
        return expr->is_complete();
    });
}

// -- Identifier ------------------------------------------------------------

Identifier::Identifier(Span location, std::string name)
    : Expression(location)
    , m_identifier(std::move(name))
{
}

std::string const& Identifier::name() const
{
    return m_identifier;
}

std::string Identifier::attributes() const
{
    return format(R"(name="{}")", name());
}

std::string Identifier::to_string() const
{
    return format("{}", name());
}

// -- Variable --------------------------------------------------------------

Variable::Variable(Span location, std::string name)
    : Identifier(location, std::move(name))
{
}

// -- BinaryExpression ------------------------------------------------------

BinaryExpression::BinaryExpression(std::shared_ptr<Expression> lhs, Token op, std::shared_ptr<Expression> rhs)
    : Expression(lhs->location().merge(rhs->location()))
    , m_lhs(std::move(lhs))
    , m_operator(op)
    , m_rhs(std::move(rhs))
{
}

std::string BinaryExpression::attributes() const
{
    return format(R"(operator="{}")", m_operator.value());
}

Nodes BinaryExpression::children() const
{
    return { m_lhs, m_rhs };
}

std::string BinaryExpression::to_string() const
{
    return format("{} {} {}", lhs()->to_string(), op().value(), rhs()->to_string());
}

std::shared_ptr<Expression> const& BinaryExpression::lhs() const
{
    return m_lhs;
}

std::shared_ptr<Expression> const& BinaryExpression::rhs() const
{
    return m_rhs;
}

Token BinaryExpression::op() const
{
    return m_operator;
}

bool BinaryExpression::is_complete() const
{
    return (m_lhs != nullptr && m_lhs->is_complete() && m_operator.code() != TokenCode::Unknown && m_rhs != nullptr && m_rhs->is_complete());
}

// -- UnaryExpression -------------------------------------------------------

UnaryExpression::UnaryExpression(Token op, std::shared_ptr<Expression> operand)
    : Expression(op.location().merge(operand->location()))
    , m_operator(op)
    , m_operand(std::move(operand))
{
}

std::string UnaryExpression::attributes() const
{
    return format(R"(operator="{}")", m_operator.value());
}

Nodes UnaryExpression::children() const
{
    return { m_operand };
}

std::string UnaryExpression::to_string() const
{
    return format("{} {}", op().value(), operand()->to_string());
}

Token UnaryExpression::op() const
{
    return m_operator;
}

std::shared_ptr<Expression> const& UnaryExpression::operand() const
{
    return m_operand;
}

bool UnaryExpression::is_complete() const
{
    return (m_operand != nullptr && m_operand->is_complete() && m_operator.code() != TokenCode::Unknown);
}

}
