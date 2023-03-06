/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App/Scratch.h>
#include <Scribble/Interp/CommandAdapter.h>
#include <Scribble/Interp/Interpreter.h>
#include <Scribble/Interp/Value.h>
#include <Scribble/Interp/Function.h>
#include <Scribble/Context.h>

namespace Scratch::Interp {

using namespace Obelix;
using namespace Scratch::Scribble;
using InterpreterContext = Context<Value>;

ProcessResult interpret(std::shared_ptr<Project> const& project)
{
    ProcessResult result;
    InterpreterContext ctx;

    if (auto res = ctx.declare("set-fixed-width-font", Value { std::make_shared<CommandAdapter>("set-fixed-width-font", *Scratch::scratch().command("set-fixed-width-font")) } ); res.is_error()) {
        result = res.error();
        return result;
    }

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

NODE_PROCESSOR(ExpressionStatement)
{
    auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
    auto expr = TRY_AND_CAST(ExpressionResult, stmt->expression(), ctx);
    return std::make_shared<ExpressionResult>(stmt->location(), expr->value());
}

NODE_PROCESSOR(VariableDeclaration)
{
    auto decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
    if (ctx.contains(decl->identifier()->name()))
        return SyntaxError { tree->location(), ErrorCode::VariableAlreadyDeclared };

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
    auto lhs = TRY_AND_TRY_CAST(ExpressionResult, expr->lhs(), ctx);
    auto rhs_node = TRY_AND_CAST(SyntaxNode, expr->rhs(), ctx);
    auto rhs = std::dynamic_pointer_cast<ExpressionResult>(rhs_node);

    switch (expr->op().code()) {
    case TokenCode::Equals: {
        if (expr->lhs()->node_type() != SyntaxNodeType::Identifier)
            return SyntaxError { expr->location(), ErrorCode::CannotAssignToRValue, expr->lhs() };
        auto ident = std::dynamic_pointer_cast<Identifier>(expr->lhs());
        ctx.set(ident->name(), rhs->value());
        return rhs;
    }

    case TokenCode::OpenParen: {
        if (lhs->value().is_error())
            return SyntaxError { expr->location(), *lhs->value().to_error(), expr->lhs() };
        if (!lhs->value().is_function())
            return SyntaxError { expr->location(), ErrorCode::FunctionUndefined, expr->lhs() };
        Values args;
        if (rhs_node->node_type() == SyntaxNodeType::ExpressionResultList) {
            args = std::dynamic_pointer_cast<ExpressionResultList>(rhs_node)->values();
        } else {
            args = { rhs->value() };
        }
        auto f = *(lhs->value().to_function());
        return std::make_shared<ExpressionResult>(expr->location(), f->execute(args));
    }

    case TokenCode::Plus: {
        auto res = lhs->value().add(rhs->value());
        if (res.is_error())
            return SyntaxError { expr->location(), *res.to_error() };
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case TokenCode::Minus: {
        auto res = lhs->value().subtract(rhs->value());
        if (res.is_error())
            return SyntaxError { expr->location(), *res.to_error() };
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    default:
        fatal("Unimplemented operator {}", expr->op().value());
    }
}

NODE_PROCESSOR(Variable)
{
    auto ident = std::dynamic_pointer_cast<Identifier>(tree);
    auto value_maybe = ctx.get(ident->name());
    if (!value_maybe.has_value())
        value_maybe = ErrorCode::UndeclaredVariable;
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
    return std::make_shared<ExpressionResult>(literal->location(), Value(literal->string()));
}

NODE_PROCESSOR(ExpressionList)
{
    auto list = std::dynamic_pointer_cast<ExpressionList>(tree);
    Values values;
    for (auto const& expr : list->expressions()) {
        auto value = TRY_AND_CAST(ExpressionResult, expr, ctx);
        if (value->value().is_error())
            return SyntaxError { expr->location(), *value->value().to_error() };
        values.push_back(value->value());
    }
    return std::make_shared<ExpressionResultList>(list->location(), values);
}

}
