/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Scribble/Syntax/Syntax.h>
#include <Scribble/Syntax/Expression.h>

namespace Scratch::Scribble {

ABSTRACT_NODE_CLASS(Literal, Expression)
protected:
    Literal(Token);

public:
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] Token const& token() const;

private:
    Token m_token;
};

NODE_CLASS(IntLiteral, Literal)
public:
    IntLiteral(Token);
};

NODE_CLASS(CharLiteral, Literal)
public:
    CharLiteral(Token);
};

NODE_CLASS(FloatLiteral, Literal)
public:
    FloatLiteral(Token);
};

NODE_CLASS(StringLiteral, Literal)
public:
    StringLiteral(Token);
};

NODE_CLASS(BooleanLiteral, Literal)
public:
    BooleanLiteral(Token);
};

}
