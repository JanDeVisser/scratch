/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Interp/Interpreter.h>
#include <Scribble/Interp/Value.h>
#include <Scribble/Context.h>

namespace Scratch::Interp {

using namespace Obelix;
using namespace Scratch::Scribble;
using InterpreterContext = Context<Value, bool>;

ProcessResult interpret(std::shared_ptr<Project> const& project)
{
    Context<Value, bool> ctx;
    ProcessResult result;
    process(project, ctx, result);
    return result;
}

}

namespace Scratch::Scribble {

using namespace Scratch::Interp;

INIT_NODE_PROCESSOR(InterpreterContext)

NODE_PROCESSOR(Project)
{
    std::shared_ptr<Project> project = std::dynamic_pointer_cast<Project>(tree);

    std::shared_ptr<Module> main;
    std::shared_ptr<ExpressionResult> res;
    for (auto const& mod : project->modules()) {
        if (mod->name() == project->main_module()) {
            main = mod;
            continue;
        }
        res = TRY_AND_CAST(ExpressionResult, mod, ctx);
    }
    return TRY_AND_CAST(ExpressionResult, res, ctx);
}

NODE_PROCESSOR(Block)
{
    std::shared_ptr<Block> block = std::dynamic_pointer_cast<Block>(tree);

    std::shared_ptr<ExpressionResult> res;
    for (auto const& statement : block->statements()) {
        res = TRY_AND_CAST(ExpressionResult, statement, ctx);
    }
    return res;
}

ALIAS_NODE_PROCESSOR(Module, Block);

NODE_PROCESSOR(VariableDeclaration)
{
    auto decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
    std::shared_ptr<ExpressionResult> expr;
    Value v;

    if (decl->expression() != nullptr) {
        expr = TRY_AND_CAST(ExpressionResult, decl->expression(), ctx);
        v = expr->value();
    } else {
        expr = std::make_shared<ExpressionResult>(decl->location(), v);
    }
    TRY_RETURN(ctx.declare(decl->identifier()->name(), v));
    return expr;
}

NODE_PROCESSOR(BinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
    auto rhs = TRY_AND_CAST(ExpressionResult, expr->rhs(), ctx);

    if (expr->op().code() == TokenCode::Equals) {
        if (expr->lhs()->node_type() != SyntaxNodeType::Identifier)
            return SyntaxError { expr->location(), ErrorCode::CannotAssignToRValue, expr->lhs() };
        auto ident = std::dynamic_pointer_cast<Identifier>(expr->lhs());
        ctx.set(ident->name(), rhs->value());
        return rhs;
    }

    auto lhs = TRY_AND_CAST(ExpressionResult, expr->lhs(), ctx);

    switch (expr->op().code()) {
    case TokenCode::Plus: {
        auto res_maybe = lhs->value().add(rhs->value());
        if (res_maybe.is_error())
            return SyntaxError { expr->location(), res_maybe.error() };
        return std::make_shared<ExpressionResult>(expr->location(), res_maybe.value());
    };
    case TokenCode::Minus: {
        auto res_maybe = lhs->value().subtract(rhs->value());
        if (res_maybe.is_error())
            return SyntaxError { expr->location(), res_maybe.error() };
        return std::make_shared<ExpressionResult>(expr->location(), res_maybe.value());
    };
    default:
        fatal("Unimplemented operator {}", expr->op().value());
    }
}

NODE_PROCESSOR(Identifier)
{
    auto ident = std::dynamic_pointer_cast<Identifier>(tree);
    auto value_maybe = ctx.get(ident->name());
    if (!value_maybe.has_value())
        return SyntaxError { ident->location(), ErrorCode::UndeclaredVariable, ident->name() };
    return std::make_shared<ExpressionResult>(ident->location(), value_maybe.value());
}

NODE_PROCESSOR(IntLiteral)
{
    auto literal = std::dynamic_pointer_cast<IntLiteral>(tree);
    return std::make_shared<ExpressionResult>(literal->location(), Value(token_value<long>(literal->token()).value()));
}

NODE_PROCESSOR(StringLiteral)
{
    auto literal = std::dynamic_pointer_cast<StringLiteral>(tree);
    return std::make_shared<ExpressionResult>(literal->location(), Value(literal->token().value()));
}

}
