/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <Scribble/Interp/Value.h>
#include <Scribble/Syntax/Syntax.h>

namespace Scratch::Interp {

using namespace std::literals;
using namespace Obelix;
using namespace Scratch::Scribble;

NODE_CLASS(ExpressionResult, SyntaxNode)
public:
    ExpressionResult(Span, Value);
    std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Value const& value() const;
private:
    Value m_value;
};

NODE_CLASS(ExpressionResultList, SyntaxNode)
public:
    ExpressionResultList(Span, Values);
    std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Values const& values() const;
private:
    Values m_values;
};

}
