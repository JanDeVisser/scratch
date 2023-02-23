/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Scribble/Syntax/Syntax.h>
#include <Scribble/Syntax/Expression.h>
#include <Scribble/Syntax/Statement.h>

namespace Scratch::Scribble {

NODE_CLASS(VariableDeclaration, Statement)
public:
    VariableDeclaration(Span, std::shared_ptr<Identifier>, std::shared_ptr<Expression> = nullptr, bool = false);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<Identifier> const& identifier() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] bool is_typed() const;
    [[nodiscard]] bool is_const() const;
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const;

private:
    std::shared_ptr<Identifier> m_identifier;
    bool m_const { false };
    std::shared_ptr<Expression> m_expression;
};

}
