/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Syntax/Literal.h>

namespace Scratch::Scribble {

extern_logging_category(scribble);

// -- BooleanLiteral --------------------------------------------------------

BooleanLiteral::BooleanLiteral(Token token)
    : Literal(std::move(token))
{
}

// -- CharLiteral -----------------------------------------------------------

CharLiteral::CharLiteral(Token token)
    : Literal(std::move(token))
{
}

// -- FloatLiteral ----------------------------------------------------------

FloatLiteral::FloatLiteral(Token token)
    : Literal(std::move(token))
{
}
// -- IntLiteral ------------------------------------------------------------

IntLiteral::IntLiteral(Token token)
    : Literal(std::move(token))
{
}

// -- Literal ---------------------------------------------------------------

Literal::Literal(Token token)
    : Expression(token.location())
    , m_token(std::move(token))
{
}

Token const& Literal::token() const
{
    return m_token;
}

std::string Literal::attributes() const
{
    return format(R"(value="{}")", token().value());
}

std::string Literal::to_string() const
{
    return std::string(token().value());
}

// -- StringLiteral ---------------------------------------------------------

StringLiteral::StringLiteral(Token token)
    : Literal(std::move(token))
{
}

}
