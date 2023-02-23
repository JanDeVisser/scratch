/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Scribble/Syntax/Syntax.h>

namespace Scratch::Scribble {

ABSTRACT_NODE_CLASS(Expression, SyntaxNode)
public:
    explicit Expression(Span = {});
};

using Expressions = std::vector<std::shared_ptr<Expression>>;

NODE_CLASS(ExpressionList, Expression)
public:
    ExpressionList(Span, Expressions);
    [[nodiscard]] Expressions const& expressions() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
private:
    Expressions m_expressions;
};

NODE_CLASS(Identifier, Expression)
public:
    explicit Identifier(Span, std::string);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_identifier;
};

using Identifiers = std::vector<std::shared_ptr<Identifier>>;

NODE_CLASS(Variable, Identifier)
public:
    Variable(Span, std::string);
};

NODE_CLASS(BinaryExpression, Expression)
public:
    BinaryExpression(std::shared_ptr<Expression>, Token, std::shared_ptr<Expression>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

    [[nodiscard]] std::shared_ptr<Expression> const& lhs() const;
    [[nodiscard]] std::shared_ptr<Expression> const& rhs() const;
    [[nodiscard]] Token op() const;

private:
    std::shared_ptr<Expression> m_lhs;
    Token m_operator;
    std::shared_ptr<Expression> m_rhs;
};

NODE_CLASS(UnaryExpression, Expression)
public:
    UnaryExpression(Token, std::shared_ptr<Expression>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] Token op() const;
    [[nodiscard]] std::shared_ptr<Expression> const& operand() const;

private:
    Token m_operator;
    std::shared_ptr<Expression> m_operand;
};

}
