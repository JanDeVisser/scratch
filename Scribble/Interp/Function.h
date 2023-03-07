/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Scribble/Interp/Interpreter.h>
#include <Scribble/Interp/Value.h>
#include <Scribble/Syntax/Function.h>

namespace Scratch::Interp {

class Function {
public:
    Function(std::string);
    virtual ~Function() = default;
    std::string const& name() const;
    [[nodiscard]] virtual Value execute(std::vector<Value> const&, InterpreterContext&) const = 0;
    [[nodiscard]] virtual std::string to_string() const;
private:
    std::string m_name;
};

using pFunction = std::shared_ptr<Function>;

class ScribbleFunction : public Function {
public:
    ScribbleFunction(std::string, pFunctionDef);
    [[nodiscard]] Value execute(std::vector<Value> const&, InterpreterContext&) const override;
    [[nodiscard]] pFunctionDef const& function() const;

private:
    pFunctionDef m_function;
};

using BuiltInImpl = std::function<Value(Values, InterpreterContext&)>;

class BuiltIn : public Function {
public:
    BuiltIn(std::string, BuiltInImpl const&);
    [[nodiscard]] Value execute(std::vector<Value> const&, InterpreterContext&) const override;
private:
    BuiltInImpl const& m_impl;
};

}
