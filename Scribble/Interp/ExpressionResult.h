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

NODE_CLASS(ExpressionResult, SyntaxNode)
public:
    explicit ExpressionResult(Span, Value);
    std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Value const& value() const;
private:
    Value m_value;
};

}
