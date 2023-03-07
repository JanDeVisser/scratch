/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Interp/Function.h>
#include <Scribble/Processor.h>

namespace Scratch::Interp {
Function::Function(std::string name)
    : m_name(name)
{
}

std::string const& Function::name() const
{
    return m_name;
}

std::string Function::to_string() const
{
    return name();
}

ScribbleFunction::ScribbleFunction(std::string name, pFunctionDef function)
    : Function(std::move(name))
    , m_function(std::move(function))
{
}

Value ScribbleFunction::execute(std::vector<Value> const& arguments, InterpreterContext& ctx) const
{
    auto params = m_function->parameters();
    if (arguments.size() < params.size())
        return Value {};
    InterpreterContext function_ctx(ctx);
    for (auto ix = 0u; ix < params.size(); ++ix) {
        auto const& arg = arguments[ix];
        auto const& param = params[ix];
        if (auto err = function_ctx.declare(param->name(), arg); err.is_error())
            return Value(ErrorCode::InternalError); // FIXME
    }
    ProcessResult result;
    auto ret_maybe = try_and_cast<ExpressionResult>(m_function->statement(), function_ctx, result);
    if (ret_maybe.is_error())
        return Value(ErrorCode::ExecutionError); // FIXME
    return ret_maybe.value()->value();
}

pFunctionDef const& ScribbleFunction::function() const
{
    return m_function;
}

BuiltIn::BuiltIn(std::string name, BuiltInImpl const& impl)
    : Function(std::move(name))
    , m_impl(impl)
{
}

Value BuiltIn::execute(std::vector<Value> const& args, InterpreterContext& ctx) const
{
    return m_impl(args, ctx);
}

}
