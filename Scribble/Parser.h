/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <map>
#include <set>

#include <lexer/BasicParser.h>

#include <Scribble/Context.h>
#include <Scribble/Processor.h>
#include <Scribble/Syntax/Forward.h>
#include <Scribble/Scribble.h>

using namespace Obelix;

namespace Scratch::Scribble {

struct ParserContext {
    std::set<std::string> modules;
};

template<>
inline ParserContext& make_subcontext(ParserContext& ctx)
{
    return ctx;
}

enum class OperandKind {
    None,
    Value,
    Type
};

struct OperatorDef {
    TokenCode op;
    OperandKind lhs_kind;
    OperandKind rhs_kind;
    int precedence;
    OperandKind unary_kind { OperandKind::None };
    int unary_precedence { -1 };
};

enum class Associativity {
    LeftToRight,
    RightToLeft,
};

class Parser {
public:
    explicit Parser(ParserContext&);
    std::shared_ptr<Module> parse(bool = false);
    [[nodiscard]] Scribble const& lexer() const;

    ErrorOr<void,SystemError> read_file(std::string const&, BufferLocator* locator = nullptr);
    void assign(StringBuffer&&);
    void assign(std::shared_ptr<StringBuffer>);
    void assign(std::string const&);
    void assign(std::vector<std::string> const&);

protected:
    Token const& peek();
    Token const& lex();
    Token const& replace(Token const&);
    std::optional<Token const> match(TokenCode, char const* = nullptr);
    Token const& skip(TokenCode);
    TokenCode current_code();
    bool expect(TokenCode, char const* = nullptr);
    bool expect(char const*, char const* = nullptr);

private:
    std::shared_ptr<Statement> parse_statement();
    std::shared_ptr<Statement> parse_top_level_statement();
    void parse_statements(Statements&, bool = false);
    std::shared_ptr<Block> parse_block(Statements&);
    std::shared_ptr<Statement> parse_function_definition(Token const&);
    std::shared_ptr<IfStatement> parse_if_statement(Token const&);
    std::shared_ptr<SwitchStatement> parse_switch_statement(Token const&);
    std::shared_ptr<WhileStatement> parse_while_statement(Token const&);
    std::shared_ptr<ForStatement> parse_for_statement(Token const&);
    std::shared_ptr<VariableDeclaration> parse_variable_declaration(Token const&, bool);
    std::shared_ptr<Import> parse_import_statement(Token const&);
    std::shared_ptr<Expression> parse_expression();

    std::shared_ptr<Expression> parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence);
    std::shared_ptr<Expression> parse_primary_expression();

    Scribble m_lexer { true };
    ParserContext& m_ctx;
    std::vector<std::string> m_modules;
    std::string m_current_module;
};

[[nodiscard]] std::string sanitize_module_name(std::string const&);
[[nodiscard]] ProcessResult compile_project(std::string const&);
[[nodiscard]] ProcessResult compile_project(std::string const&, std::shared_ptr<StringBuffer>);

}
