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
#include <Scribble/Scribble.h>

namespace Scratch::Interp {

using namespace Obelix;
using namespace Scratch::Scribble;

ErrorOr<void, SyntaxError> register_builtin(InterpreterContext& ctx, std::string const& name, BuiltInImpl const& impl)
{
    TRY_RETURN(ctx.declare(name, Value { std::make_shared<BuiltIn>(name, impl) } ));
    return {};
}

ProcessResult interpret(std::shared_ptr<Project> const& project)
{
    ProcessResult result;
    InterpreterContext ctx;

    TRY_RETURN(ctx.declare("set-fixed-width-font", Value { std::make_shared<CommandAdapter>("set-fixed-width-font", *Scratch::scratch().command("set-fixed-width-font")) } ));
    TRY_RETURN(register_builtin(ctx, "string-length", [](Values const& args, InterpreterContext&) -> Value {
        if (args.empty() && args.size() > 1)
            return Value { ErrorCode::ArgumentCountMismatch };
        if (args[0].type() != ValueType::Text)
            return Value { ErrorCode::ArgumentTypeMismatch };
        return Value { args[0].to_string().length() };
    }));

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
    InterpreterContext block_ctx(ctx);
    for (auto const& statement : block->statements()) {
        res = TRY_AND_CAST(ExpressionResult, statement, block_ctx);
        switch ((*block_ctx).type) {
        case StatementResult::StatementResultType::Return:
            return std::make_shared<ExpressionResult>(block->location(), (*block_ctx).payload);
        case StatementResult::StatementResultType::Break:
        case StatementResult::StatementResultType::Continue:
            return res;
        default:
            break;
        }
    }
    return res;
}

ALIAS_NODE_PROCESSOR(Module, Block);

NODE_PROCESSOR(FunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
    if (auto decl_maybe = ctx.declare(func_def->name(), Value { std::make_shared<ScribbleFunction>(func_def->name(), func_def) } ); decl_maybe.is_error())
        return decl_maybe.error();
    return std::make_shared<ExpressionResult>(func_def->location(), Value { });
}

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
        return std::make_shared<ExpressionResult>(expr->location(), f->execute(args, ctx));
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

    case TokenCode::Asterisk: {
        auto res = lhs->value().multiply(rhs->value());
        if (res.is_error())
            return SyntaxError { expr->location(), *res.to_error() };
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case TokenCode::Slash: {
        auto res = lhs->value().divide(rhs->value());
        if (res.is_error())
            return SyntaxError { expr->location(), *res.to_error() };
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case TokenCode::Percent: {
        auto res = lhs->value().modulo(rhs->value());
        if (res.is_error())
            return SyntaxError { expr->location(), *res.to_error() };
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case TokenCode::EqualsTo: {
        auto res = Value(lhs->value() == rhs->value());
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case TokenCode::NotEqualTo: {
        auto res = Value(lhs->value() != rhs->value());
        return std::make_shared<ExpressionResult>(expr->location(), res);
    }

    case TokenCode::GreaterThan: {
        auto res = Value(lhs->value() > rhs->value());
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case TokenCode::GreaterEqualThan: {
        auto res = Value(lhs->value() >= rhs->value());
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case TokenCode::LessThan: {
        auto res = Value(lhs->value() < rhs->value());
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case TokenCode::LessEqualThan: {
        auto res = Value(lhs->value() <= rhs->value());
        return std::make_shared<ExpressionResult>(expr->location(), res);
    };

    case Scribble::KeywordRange: {
        if (lhs->value().type() != ValueType::Integer || rhs->value().type() != ValueType::Integer)
            return SyntaxError { expr->location(), ErrorCode::TypeMismatch };
        return std::make_shared<ExpressionResultList>(expr->location(), Values { lhs->value(), rhs->value() });
    }

    default:
        return SyntaxError { expr->location(), ErrorCode::InternalError, format("Unimplemented operator {}", expr->op().value()) };
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

NODE_PROCESSOR(IfStatement)
{
    auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
    for (auto const& branch : if_stmt->branches()) {
        auto cond = TRY_AND_CAST(ExpressionResult, branch->condition(), ctx);

        if (auto match_maybe = cond->value().to_bool(); match_maybe) {
            if (*match_maybe)
                return TRY_AND_CAST(ExpressionResult, branch->statement(), ctx);
        } else {
            return SyntaxError { if_stmt->location(), ErrorCode::TypeMismatch };
        }
    }
    if (if_stmt->else_stmt() != nullptr)
        return TRY_AND_CAST(ExpressionResult, if_stmt->else_stmt(), ctx);
    return std::make_shared<ExpressionResult>(if_stmt->location(), Value {});
}

NODE_PROCESSOR(SwitchStatement)
{
    auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
    for (auto const& case_stmt : switch_stmt->cases()) {
        auto match_expr = std::make_shared<BinaryExpression>(switch_stmt->expression(), Token { {}, TokenCode::EqualsTo, "==" }, case_stmt->condition());
        auto match_value = TRY_AND_CAST(ExpressionResult, match_expr, ctx);
        if (auto match_maybe = match_value->value().to_bool(); match_maybe) {
            if (*match_maybe)
                return TRY_AND_CAST(ExpressionResult, case_stmt->statement(), ctx);
        } else {
            return SyntaxError { case_stmt->location(), ErrorCode::TypeMismatch };
        }
    }
    if (switch_stmt->default_case() != nullptr)
        return TRY_AND_CAST(ExpressionResult, switch_stmt->default_case()->statement(), ctx);
    return std::make_shared<ExpressionResult>(switch_stmt->location(), Value {});
}

NODE_PROCESSOR(ExpressionResult)
{
    return tree;
}

NODE_PROCESSOR(WhileStatement)
{
    auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);

    Value res;
    do {
        auto cond = TRY_AND_CAST(ExpressionResult, while_stmt->condition(), ctx);
        auto cond_maybe = cond->value().to_bool();
        if (!cond_maybe)
            return SyntaxError { while_stmt->location(), ErrorCode::TypeMismatch };
        if (!cond_maybe.value())
            return std::make_shared<ExpressionResult>(while_stmt->location(), res);
        auto stmt_result = TRY_AND_CAST(ExpressionResult, while_stmt->statement(), ctx);
        if ((*ctx).type == StatementResult::StatementResultType::Break)
            return std::make_shared<ExpressionResult>(while_stmt->location(), res);
        if ((*ctx).type == StatementResult::StatementResultType::Return) {
            return std::make_shared<ExpressionResult>(while_stmt->location(), (*ctx).payload);
        }
        res = stmt_result->value();
    } while (true);
}

NODE_PROCESSOR(ForStatement)
{
    auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);

    Value res;
    auto range_expr = TRY_AND_CAST(ExpressionResultList, for_stmt->range(), ctx);
    auto current = *(range_expr->values()[0].to_int<long>());
    auto upper_bound = *(range_expr->values()[1].to_int<long>());
    InterpreterContext for_ctx(ctx);
    TRY_RETURN(for_ctx.declare(for_stmt->variable()->name(), Value(current)));
    while (current < upper_bound) {
        for_ctx.set(for_stmt->variable()->name(), Value(current));
        auto stmt_result = TRY_AND_CAST(ExpressionResult, for_stmt->statement(), for_ctx);
        if ((*for_ctx).type == StatementResult::StatementResultType::Break)
            break;
        if ((*for_ctx).type == StatementResult::StatementResultType::Return) {
            ctx = *for_ctx;
            return std::make_shared<ExpressionResult>(for_stmt->location(), (*ctx).payload);
        }
        res = stmt_result->value();
        ++current;
    }
    return std::make_shared<ExpressionResult>(for_stmt->location(), res);
}

NODE_PROCESSOR(Return)
{
    auto ret_stmt = std::dynamic_pointer_cast<Return>(tree);
    if (ret_stmt->expression()) {
        auto ret_val = TRY_AND_CAST(ExpressionResult, ret_stmt->expression(), ctx);
        *ctx = { StatementResult::StatementResultType::Return, ret_val->value() };
    } else {
        *ctx = { StatementResult::StatementResultType::Return, Value {} };
    }
    return std::make_shared<ExpressionResult>(ret_stmt->location(), (*ctx).payload);
}

NODE_PROCESSOR(Break)
{
    *ctx = { StatementResult::StatementResultType::Break, Value {} };
    return std::make_shared<ExpressionResult>(tree->location(), Value {});
}

NODE_PROCESSOR(Continue)
{
    *ctx = { StatementResult::StatementResultType::Continue, Value {} };
    return std::make_shared<ExpressionResult>(tree->location(), Value {});
}

}
