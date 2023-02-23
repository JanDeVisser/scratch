/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>

#include <core/Error.h>
#include <Scribble/Context.h>
#include <Scribble/Syntax/ControlFlow.h>
#include <Scribble/Syntax/Expression.h>
#include <Scribble/Syntax/Function.h>
#include <Scribble/Syntax/Literal.h>
#include <Scribble/Syntax/Statement.h>
#include <Scribble/Syntax/Syntax.h>
#include <Scribble/Syntax/Variable.h>

namespace Scratch::Scribble {

extern_logging_category(scribble);

using ErrorOrNode = ErrorOr<std::shared_ptr<SyntaxNode>, SyntaxError>;

template<class NodeClass>
using ErrorOrTypedNode = ErrorOr<std::shared_ptr<NodeClass>, SyntaxError>;

using Errors = std::vector<SyntaxError>;

class ProcessResult {
public:
    ProcessResult() = default;
    ProcessResult(ProcessResult const& other) = default;

    ProcessResult(pSyntaxNode const& node)
        : m_result(node)
    {
    }

    ProcessResult(SyntaxError const& err)
        : m_errors({err})
    {
    }

    [[nodiscard]] Errors const& errors() const { return m_errors; }
    [[nodiscard]] Errors const& warnings() const { return m_warnings; }
    [[nodiscard]] bool is_error() const { return !m_errors.empty(); }
    [[nodiscard]] bool has_value() const { return m_errors.empty() && (m_result != nullptr); }
    [[nodiscard]] pSyntaxNode value() const { return m_result; }
    void value(pSyntaxNode const& node) { m_result = node; }
    [[nodiscard]] SyntaxError error() const { return m_errors.back(); }

    void error(SyntaxError const& err)
    {
        if (m_errors.empty() || m_errors.back() != err)
            m_errors.push_back(err);
    }

    void warn(SyntaxError const& w)
    {
        m_warnings.push_back(w);
    }

    ProcessResult& operator=(pSyntaxNode const& result)
    {
        value(result);
        return *this;
    }

    ProcessResult& operator=(SyntaxError const& err)
    {
        error(err);
        return *this;
    }

    ProcessResult& operator+=(ProcessResult const& other)
    {
        m_result = other.m_result;
        for (auto const& e : other.m_errors)
            m_errors.push_back(e);
        for (auto const& w : other.m_warnings)
            m_warnings.push_back(w);
        return *this;
    }

private:
    pSyntaxNode m_result { nullptr };
    Errors m_errors {};
    Errors m_warnings {};
};

#define TRY_AND_TRY_CAST(cls, expr, ctx)                          \
    ({                                                            \
        auto __casted = try_and_try_cast<cls>(expr, ctx, result); \
        if (__casted.is_error())                                  \
            return __casted.error();                              \
        std::shared_ptr<cls> __ret = __casted.value();            \
        __ret;                                                    \
    })

#define TRY_AND_CAST(cls, expr, ctx)                          \
    ({                                                        \
        auto __casted = try_and_cast<cls>(expr, ctx, result); \
        if (__casted.is_error())                              \
            return __casted.error();                          \
        std::shared_ptr<cls> __ret = __casted.value();        \
        __ret;                                                \
    })

#define TRY_AND_TRY_CAST_RETURN(cls, expr, ctx, return_value)     \
    ({                                                            \
        auto __casted = try_and_try_cast<cls>(expr, ctx, result); \
        if (__casted.is_error())                                  \
            return __casted.error();                              \
        if (__casted.value() == nullptr)                          \
            return return_value;                                  \
        std::shared_ptr<cls> __ret = __casted.value();            \
        __ret;                                                    \
    })

template<typename Context, typename Processor>
ErrorOrNode process_tree(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, ProcessResult& result, Processor processor)
{
    if (!tree)
        return tree;

    std::shared_ptr<SyntaxNode> ret = tree;

    switch (tree->node_type()) {

    case SyntaxNodeType::Project: {
        auto compilation = std::dynamic_pointer_cast<Project>(tree);
        Modules modules;
        for (auto& module : compilation->modules()) {
            modules.push_back(TRY_AND_CAST(Module, module, ctx));
        }
        ret = make_node<Project>(modules, compilation->main_module());
        break;
    }

    case SyntaxNodeType::Module: {
        auto module = std::dynamic_pointer_cast<Module>(tree);
        ret = TRY(process_block<Module>(tree, ctx, result, processor, module->name()));
        break;
    }

    case SyntaxNodeType::Block: {
        ret = TRY(process_block<Block>(tree, ctx, result, processor));
        break;
    }

    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto func_decl = TRY_AND_CAST(FunctionDecl, func_def->declaration(), ctx);
        auto statement = func_def->statement();
        if (statement)
            statement = TRY_AND_CAST(Statement, statement, ctx);
        ret = std::make_shared<FunctionDef>(func_def->location(), func_decl, statement);
        break;
    }

    case SyntaxNodeType::FunctionDecl:
    case SyntaxNodeType::NativeFunctionDecl:
    case SyntaxNodeType::IntrinsicDecl: {
        auto func_decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
        auto identifier = TRY_AND_CAST(Identifier, func_decl->identifier(), ctx);
        Identifiers parameters;
        for (auto& param : func_decl->parameters()) {
            auto processed_param = TRY_AND_CAST(Identifier, param, ctx);
            parameters.push_back(processed_param);
        }
        switch (func_decl->node_type()) {
        case SyntaxNodeType::FunctionDecl:
            ret = std::make_shared<FunctionDecl>(func_decl->location(), func_decl->module(), identifier, parameters);
            break;
        case SyntaxNodeType::NativeFunctionDecl:
            ret = std::make_shared<NativeFunctionDecl>(func_decl->location(), func_decl->module(), identifier, parameters,
                std::dynamic_pointer_cast<NativeFunctionDecl>(func_decl)->native_function_name());
            break;
        case SyntaxNodeType::IntrinsicDecl:
            ret = std::make_shared<IntrinsicDecl>(func_decl->location(), func_decl->module(), identifier, parameters);
            break;
        default:
            fatal("Unreachable");
        }
        break;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, stmt->expression(), ctx);
        ret = std::make_shared<ExpressionStatement>(expr);
        break;
    }

    case SyntaxNodeType::ExpressionList: {
        auto list = std::dynamic_pointer_cast<ExpressionList>(tree);
        Expressions expressions;
        for (auto const& expr : list->expressions()) {
            auto processed = TRY_AND_CAST(Expression, expr, ctx);
            expressions.push_back(processed);
        }
        return std::make_shared<ExpressionList>(list->location(), expressions);
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);

        auto lhs = TRY_AND_CAST(Expression, expr->lhs(), ctx);
        auto rhs = TRY_AND_CAST(Expression, expr->rhs(), ctx);
        ret = std::make_shared<BinaryExpression>(lhs, expr->op(), rhs);
        break;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, expr->operand(), ctx);
        ret = std::make_shared<UnaryExpression>(expr->op(), operand);
        break;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, var_decl->expression(), ctx);
        ret = std::make_shared<VariableDeclaration>(var_decl->location(), var_decl->identifier(), expr);
        break;
    }

    case SyntaxNodeType::Return: {
        auto return_stmt = std::dynamic_pointer_cast<Return>(tree);
        auto expr = TRY_AND_CAST(Expression, return_stmt->expression(), ctx);
        ret = std::make_shared<Return>(return_stmt->location(), expr, return_stmt->return_error());
        break;
    }

    case SyntaxNodeType::Branch: {
        auto branch = std::dynamic_pointer_cast<Branch>(tree);
        std::shared_ptr<Expression> condition { nullptr };
        if (branch->condition())
            condition = TRY_AND_CAST(Expression, branch->condition(), ctx);
        auto statement = TRY_AND_CAST(Statement, branch->statement(), ctx);
        ret = std::make_shared<Branch>(branch, condition, statement);
        break;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        Branches branches;
        for (auto& branch : if_stmt->branches()) {
            auto branch_maybe = processor(branch, ctx, result);
            if (branch_maybe.has_value())
                branches.push_back(std::dynamic_pointer_cast<Branch>(branch_maybe.value()));
        }
        ret = std::make_shared<IfStatement>(if_stmt->location(), branches);
        break;
    }

    case SyntaxNodeType::WhileStatement: {
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        auto condition = TRY_AND_CAST(Expression, while_stmt->condition(), ctx);
        auto stmt = TRY_AND_CAST(Statement, while_stmt->statement(), ctx);
        if ((condition != while_stmt->condition()) || (stmt != while_stmt->statement()))
            ret = std::make_shared<WhileStatement>(while_stmt->location(), condition, stmt);
        break;
    }

    case SyntaxNodeType::ForStatement: {
        auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);
        auto variable = TRY_AND_CAST(Variable, for_stmt->variable(), ctx);
        auto range = TRY_AND_CAST(Expression, for_stmt->range(), ctx);
        auto stmt = TRY_AND_CAST(Statement, for_stmt->statement(), ctx);
        ret = std::make_shared<ForStatement>(for_stmt->location(), variable, range, stmt);
        break;
    }

    case SyntaxNodeType::CaseStatement: {
        auto stmt = std::dynamic_pointer_cast<CaseStatement>(tree);
        std::shared_ptr<Expression> condition { nullptr };
        if (stmt->condition())
            condition = TRY_AND_CAST(Expression, stmt->condition(), ctx);
        auto statement = TRY_AND_CAST(Statement, stmt->statement(), ctx);
        ret = std::make_shared<CaseStatement>(stmt, condition, statement);
        break;
    }

    case SyntaxNodeType::DefaultCase: {
        auto stmt = std::dynamic_pointer_cast<DefaultCase>(tree);
        std::shared_ptr<Expression> condition { nullptr };
        if (stmt->condition())
            condition = TRY_AND_CAST(Expression, stmt->condition(), ctx);
        auto statement = TRY_AND_CAST(Statement, stmt->statement(), ctx);
        ret = std::make_shared<DefaultCase>(stmt, condition, statement);
        break;
    }

    case SyntaxNodeType::SwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, switch_stmt->expression(), ctx);
        CaseStatements cases;
        for (auto& case_stmt : switch_stmt->cases()) {
            cases.push_back(TRY_AND_CAST(CaseStatement, case_stmt, ctx));
        }
        auto default_case = TRY_AND_CAST(DefaultCase, switch_stmt->default_case(), ctx);
        ret = std::make_shared<SwitchStatement>(switch_stmt->location(), expr, cases, default_case);
        break;
    }

    default:
        break;
    }

    return ret;
}

template<class StmtClass, typename Context, typename Processor, typename... Args>
ErrorOrNode process_block(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, ProcessResult& result, Processor& processor, Args&&... args)
{
    auto block = std::dynamic_pointer_cast<Block>(tree);
    Context& child_ctx = make_subcontext<Context>(ctx);
    Statements statements;
    for (auto& stmt : block->statements()) {
        auto processed_or_error = processor(stmt, child_ctx, result);
        if (processed_or_error.is_error()) {
            result = processed_or_error.error();
            continue;
        }
        auto processed_node = processed_or_error.value();
        if (auto new_statement = std::dynamic_pointer_cast<Statement>(processed_node); new_statement != nullptr) {
            statements.push_back(new_statement);
        } else if (auto node_list = std::dynamic_pointer_cast<NodeList<Statement>>(processed_node); node_list != nullptr) {
            for (auto const& node : *node_list) {
                statements.push_back(node);
            }
        }
    }
    if (statements != block->statements())
        return std::make_shared<StmtClass>(tree->location(), statements, std::forward<Args>(args)...);
    else
        return tree;
}

template<typename Context, typename Processor>
ErrorOr<Expressions, SyntaxError> xform_expressions(Expressions const& expressions, Context& ctx, ProcessResult& result, Processor processor)
{
    Expressions ret;
    for (auto& expr : expressions) {
        auto new_expr = TRY(processor(expr, ctx, result));
        ret.push_back(std::dynamic_pointer_cast<Expression>(new_expr));
    }
    return ret;
}

template<typename Ctx>
ProcessResult& process(std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx, ProcessResult& result)
{
    if (!tree) {
        result.value(nullptr);
        return result;
    }
    std::string log_message;
    debug(scribble, "Process <{} {}>", tree->node_type(), tree);
    switch (tree->node_type()) {
#undef ENUM_SYNTAXNODETYPE
#define ENUM_SYNTAXNODETYPE(type)                                                           \
    case SyntaxNodeType::type: {                                                            \
        log_message = format("<{} {}> => ", #type, tree);                                   \
        ErrorOrNode processed = process_node<Ctx, SyntaxNodeType::type>(tree, ctx, result); \
        if (processed.is_error()) {                                                         \
            log_message += format("Error {}", processed.error());                           \
            result.error(processed.error());                                                \
            result = tree;                                                                  \
        } else {                                                                            \
            result = processed.value();                                                     \
            log_message += format("<{} {}>", result.value()->node_type(), result.value());  \
        }                                                                                   \
        debug(scribble, "{}", log_message);                                                \
        return result;                                                                      \
    }
        ENUMERATE_SYNTAXNODETYPES(ENUM_SYNTAXNODETYPE)
#undef ENUM_SYNTAXNODETYPE
    default:
        fatal("Unknown SyntaxNodeType '{}'", tree->node_type());
    }
}

template<typename Ctx>
ProcessResult process(std::shared_ptr<SyntaxNode> const& tree)
{
    ProcessResult result;
    Ctx ctx;
    return process<Ctx>(tree, ctx, result);
}

template<typename Ctx>
ProcessResult process(std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx)
{
    ProcessResult result;
    return process<Ctx>(tree, ctx, result);
}

template<typename Ctx, SyntaxNodeType node_type>
ErrorOrNode process_node(std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx, ProcessResult& result)
{
    debug(scribble, "Falling back to default processor for type {}", tree->node_type());
    return process_tree(tree, ctx, result, [](std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx, ProcessResult& result) {
        return process(tree, ctx, result);
    });
}

template<class Cls, class Ctx>
ErrorOrTypedNode<Cls> try_and_try_cast(pSyntaxNode const& expr, Ctx& ctx, ProcessResult& result)
{
    process(expr, ctx, result);
    if (result.is_error()) {
        debug(scribble, "Processing node results in error '{}' instead of node of type '{}'", result.error(), typeid(Cls).name());
        return result.error();
    }
    if (result.value() == nullptr)
        return nullptr;
    return std::dynamic_pointer_cast<Cls>(result.value());
}

template<class Cls, class Ctx>
ErrorOrTypedNode<Cls> try_and_cast(pSyntaxNode const& expr, Ctx& ctx, ProcessResult& result)
{
    process(expr, ctx, result);
    if (result.is_error()) {
        debug(scribble, "Processing node results in error '{}' instead of node of type '{}'", result.error(), typeid(Cls).name());
        return result.error();
    }
    if (result.value() == nullptr)
        return nullptr;
    auto casted = std::dynamic_pointer_cast<Cls>(result.value());
    if (casted == nullptr) {
        return SyntaxError { result.value()->location(), "Processing node results in unexpected type '{}' instead of '{}'", result.value()->node_type(), typeid(Cls).name() };
    }
    return casted;
}

#define INIT_NODE_PROCESSOR(context_type)                                                                                    \
    using ContextType = context_type;                                                                                        \
    ProcessResult context_type##_processor(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx, ProcessResult& result) \
    {                                                                                                                        \
        return process<context_type>(tree, ctx, result);                                                                     \
    }

#define NODE_PROCESSOR(node_type) \
    template<>                    \
    ErrorOrNode process_node<ContextType, SyntaxNodeType::node_type>(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx, ProcessResult& result)

#define ALIAS_NODE_PROCESSOR(node_type, alias_node_type)                                                                                               \
    template<>                                                                                                                                         \
    ErrorOrNode process_node<ContextType, SyntaxNodeType::node_type>(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx, ProcessResult& result) \
    {                                                                                                                                                  \
        return process_node<ContextType, SyntaxNodeType::alias_node_type>(tree, ctx, result);                                                          \
    }

}
