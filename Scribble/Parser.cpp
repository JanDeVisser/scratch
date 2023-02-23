/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>

#include <Scribble/Syntax/Expression.h>
#include <Scribble/Syntax/Syntax.h>
#include <Scribble/Parser.h>
#include <Scribble/Scribble.h>

namespace Scratch::Scribble {

using namespace Obelix;

logging_category(scribble);

ErrorOr<std::shared_ptr<Parser>, SystemError> Parser::create(ParserContext& ctx, std::string const& file_name)
{
    auto ret = std::shared_ptr<Parser>(new Parser(ctx));
    TRY_RETURN(ret->m_lexer.read_file(file_name));
    ret->m_current_module = sanitize_module_name(file_name);
    return ret;
}

Parser::Parser(ParserContext& ctx)
    : m_ctx(ctx)
{
}

Scribble const& Parser::lexer() const
{
    return m_lexer;
}

std::shared_ptr<Module> Parser::parse()
{
    if (!m_lexer.errors().empty())
        return nullptr;
    Statements statements;
    parse_statements(statements, true);
    if (m_lexer.has_errors())
        return nullptr;
    return std::make_shared<Module>(statements, m_current_module);
}

std::shared_ptr<Statement> Parser::parse_top_level_statement()
{
    debug(scribble, "Parser::parse_top_level_statement");
    auto token = m_lexer.peek();
    std::shared_ptr<Statement> ret;
    switch (token.code()) {
    case TokenCode::SemiColon:
        return std::make_shared<Pass>(m_lexer.lex().location());
    case TokenCode::OpenBrace: {
        m_lexer.lex();
        Statements statements;
        return parse_block(statements);
    }
    case Scribble::KeywordImport:
        return parse_import_statement(m_lexer.lex());
    case Scribble::KeywordVar:
    case Scribble::KeywordConst:
        return parse_variable_declaration(m_lexer.lex(), token.code() == Scribble::KeywordConst);
    case Scribble::KeywordCmd:
    case Scribble::KeywordFunc:
    case Scribble::KeywordIntrinsic:
        return parse_function_definition(m_lexer.lex());
    case TokenCode::Identifier: {
        break;
    }
    case TokenCode::CloseBrace:
    case TokenCode::EndOfFile:
        return nullptr;
    default:
        break;
    }
    auto expr = parse_expression();
    if (!expr)
        return nullptr;
    return std::make_shared<ExpressionStatement>(expr);
}

std::shared_ptr<Statement> Parser::parse_statement()
{
    debug(scribble, "Parser::parse_statement");
    auto token = m_lexer.peek();
    std::shared_ptr<Statement> ret;
    switch (token.code()) {
    case TokenCode::SemiColon:
        return std::make_shared<Pass>(m_lexer.lex().location());
    case TokenCode::OpenBrace: {
        m_lexer.lex();
        Statements statements;
        return parse_block(statements);
    }
    case Scribble::KeywordImport:
        return parse_import_statement(m_lexer.lex());
    case Scribble::KeywordIf:
        return parse_if_statement(m_lexer.lex());
    case Scribble::KeywordSwitch:
        return parse_switch_statement(m_lexer.lex());
    case Scribble::KeywordWhile:
        return parse_while_statement(m_lexer.lex());
    case Scribble::KeywordFor:
        return parse_for_statement(m_lexer.lex());
    case Scribble::KeywordVar:
    case Scribble::KeywordConst:
        return parse_variable_declaration(m_lexer.lex(), token.code() == Scribble::KeywordConst);
    case Scribble::KeywordReturn: {
        m_lexer.lex();
        auto expr = parse_expression();
        if (!expr)
            return nullptr;
        return std::make_shared<Return>(token.location(), expr);
    }
    case TokenCode::Identifier: {
        if (token.value() == "error") {
            m_lexer.lex();
            auto expr = parse_expression();
            if (!expr)
                return nullptr;
            return std::make_shared<Return>(token.location(), expr, true);
        }
        break;
    }
    case Scribble::KeywordBreak:
        return std::make_shared<Break>(m_lexer.lex().location());
    case Scribble::KeywordContinue:
        return std::make_shared<Continue>(m_lexer.lex().location());
    case TokenCode::CloseBrace:
    case TokenCode::EndOfFile:
        return nullptr;
    default: {
        break;
    }
    }
    auto expr = parse_expression();
    if (!expr)
        return nullptr;
    return std::make_shared<ExpressionStatement>(expr);
}

void Parser::parse_statements(Statements& block, bool top_level)
{
    while (true) {
        auto statement = (top_level) ? parse_top_level_statement() : parse_statement();
        if (!statement)
            break;
        block.push_back(statement);
    }
}

std::shared_ptr<Block> Parser::parse_block(Statements& block)
{
    auto token = m_lexer.peek();
    parse_statements(block);
    if (!m_lexer.expect(TokenCode::CloseBrace)) {
        return nullptr;
    }
    return std::make_shared<Block>(token.location(), block);
}

std::shared_ptr<Statement> Parser::parse_function_definition(Token const& func_token)
{
    auto name_maybe = m_lexer.match(TokenCode::Identifier);
    if (!name_maybe.has_value()) {
        m_lexer.add_error(m_lexer.peek(), "Expecting variable name after the 'func' keyword, got '{}'");
        return nullptr;
    }
    auto name = name_maybe.value();
    if (!m_lexer.expect(TokenCode::OpenParen, "after function name in definition")) {
        return nullptr;
    }
    Identifiers params {};
    auto done = m_lexer.current_code() == TokenCode::CloseParen;
    while (!done) {
        auto param_name_maybe = m_lexer.match(TokenCode::Identifier);
        if (!param_name_maybe.has_value()) {
            m_lexer.add_error(m_lexer.peek(), "Expected parameter name, got '{}'");
            return nullptr;
        }
        auto param_name = param_name_maybe.value();
        if (!m_lexer.expect(TokenCode::Colon))
            return nullptr;

        params.push_back(std::make_shared<Identifier>(param_name.location(), param_name.to_string()));
        switch (m_lexer.current_code()) {
        case TokenCode::Comma:
            m_lexer.lex();
            break;
        case TokenCode::CloseParen:
            done = true;
            break;
        default:
            m_lexer.add_error(m_lexer.peek(), "Syntax Error: Expected ',' or ')' in function parameter list, got '{}'", m_lexer.peek().value());
            return nullptr;
        }
    }
    m_lexer.lex(); // Eat the closing paren

    if (!m_lexer.expect(TokenCode::Colon))
        return nullptr;

    auto func_ident = std::make_shared<Identifier>(name.location(), name.to_string());
    std::shared_ptr<FunctionDecl> func_decl;
    if (m_lexer.current_code() == Scribble::KeywordLink) {
        m_lexer.lex();
        if (auto link_target_maybe = m_lexer.match(TokenCode::DoubleQuotedString, "after '->'"); link_target_maybe.has_value()) {
            return std::make_shared<NativeFunctionDecl>(name.location(), m_current_module, func_ident, params, link_target_maybe.value().to_string());
        }
        return nullptr;
    }
    if (func_token.code() == Scribble::KeywordIntrinsic) {
        return std::make_shared<IntrinsicDecl>(name.location(), m_current_module, func_ident, params);
    }
    func_decl = std::make_shared<FunctionDecl>(name.location(), m_current_module, func_ident, params);
    auto stmt = parse_statement();
    if (stmt == nullptr)
        return nullptr;
    return std::make_shared<FunctionDef>(func_token.location(), func_decl, stmt);
}

std::shared_ptr<IfStatement> Parser::parse_if_statement(Token const& if_token)
{
    auto condition = parse_expression();
    if (!condition)
        return nullptr;
    auto if_stmt = parse_statement();
    if (!if_stmt)
        return nullptr;
    Branches branches;
    while (true) {
        switch (m_lexer.current_code()) {
        case Scribble::KeywordElif: {
            auto elif_token = m_lexer.lex();
            auto elif_condition = parse_expression();
            if (!elif_condition)
                return nullptr;
            auto elif_stmt = parse_statement();
            if (!elif_stmt)
                return nullptr;
            branches.push_back(std::make_shared<Branch>(elif_token.location(), elif_condition, elif_stmt));
        } break;
        case Scribble::KeywordElse: {
            auto else_token = m_lexer.lex();
            auto else_stmt = parse_statement();
            if (!else_stmt)
                return nullptr;
            return std::make_shared<IfStatement>(if_token.location(), condition, if_stmt, branches, else_stmt);
        }
        default:
            return std::make_shared<IfStatement>(if_token.location(), condition, if_stmt, branches, nullptr);
        }
    }
}

std::shared_ptr<SwitchStatement> Parser::parse_switch_statement(Token const& switch_token)
{
    auto switch_expr = parse_expression();
    if (!switch_expr)
        return nullptr;
    if (!m_lexer.expect(TokenCode::OpenBrace, "after switch expression")) {
        return nullptr;
    }
    CaseStatements cases;
    std::shared_ptr<DefaultCase> default_case { nullptr };
    while (true) {
        switch (m_lexer.current_code()) {
        case Scribble::KeywordCase: {
            auto case_token = m_lexer.lex();
            auto expr = parse_expression();
            if (!expr)
                return nullptr;
            if (!m_lexer.expect(TokenCode::Colon, "after switch expression")) {
                return nullptr;
            }
            auto stmt = parse_statement();
            if (!stmt)
                return nullptr;
            cases.push_back(std::make_shared<CaseStatement>(case_token.location(), expr, stmt));
        } break;
        case Scribble::KeywordDefault: {
            auto default_token = m_lexer.lex();
            if (!m_lexer.expect(TokenCode::Colon, "after 'default' keyword")) {
                return nullptr;
            }
            auto stmt = parse_statement();
            if (!stmt)
                return nullptr;
            default_case = std::make_shared<DefaultCase>(default_token.location(), stmt);
            break;
        }
        case TokenCode::CloseBrace:
            m_lexer.lex();
            return std::make_shared<SwitchStatement>(switch_token.location(), switch_expr, cases, default_case);
        default:
            m_lexer.add_error(m_lexer.peek(), "Syntax Error: Unexpected token '{}' in switch statement");
            return nullptr;
        }
    }
}

std::shared_ptr<WhileStatement> Parser::parse_while_statement(Token const& while_token)
{
    if (!m_lexer.expect(TokenCode::OpenParen, " in 'while' statement"))
        return nullptr;
    auto condition = parse_expression();
    if (!condition)
        return nullptr;
    if (!m_lexer.expect(TokenCode::CloseParen, " in 'while' statement"))
        return nullptr;
    auto stmt = parse_statement();
    if (!stmt)
        return nullptr;
    return std::make_shared<WhileStatement>(while_token.location(), condition, stmt);
}

std::shared_ptr<ForStatement> Parser::parse_for_statement(Token const& for_token)
{
    if (!m_lexer.expect(TokenCode::OpenParen, " in 'for' statement"))
        return nullptr;
    auto variable = m_lexer.match(TokenCode::Identifier, " in 'for' statement");
    if (!variable.has_value())
        return nullptr;
    if (!m_lexer.expect("in", " in 'for' statement"))
        return nullptr;
    auto expr = parse_expression();
    if (!expr)
        return nullptr;
    if (!m_lexer.expect(TokenCode::CloseParen, " in 'for' statement"))
        return nullptr;
    auto stmt = parse_statement();
    if (!stmt)
        return nullptr;
    auto variable_node = std::make_shared<Variable>(variable.value().location(), variable.value().to_string());
    return std::make_shared<ForStatement>(for_token.location(), variable_node, expr, stmt);
}

std::shared_ptr<VariableDeclaration> Parser::parse_variable_declaration(Token const& var_token, bool constant)
{
    auto identifier_maybe = m_lexer.match(TokenCode::Identifier);
    if (!identifier_maybe.has_value()) {
        return nullptr;
    }
    auto identifier = identifier_maybe.value();
    auto var_ident = std::make_shared<Identifier>(identifier.location(), identifier.to_string());
    std::shared_ptr<Expression> expr { nullptr };
    if (m_lexer.current_code() == TokenCode::Equals) {
        m_lexer.lex();
        expr = parse_expression();
        if (!expr)
            return nullptr;
    } else {
        if (constant) {
            m_lexer.add_error(m_lexer.peek(), "Syntax Error: Expected expression after constant declaration, got '{}' ({})", m_lexer.peek().value(), m_lexer.peek().code_name());
            return nullptr;
        }
    }
    return std::make_shared<VariableDeclaration>(var_token.location(), var_ident, expr, constant);
}

std::shared_ptr<Import> Parser::parse_import_statement(Token const& import_token)
{
    std::string module_name;
    while (true) {
        auto identifier_maybe = m_lexer.match(TokenCode::Identifier, "in import statement");
        if (!identifier_maybe.has_value())
            return nullptr;
        module_name += identifier_maybe.value().value();
        if (m_lexer.current_code() != TokenCode::Slash)
            break;
        m_lexer.lex();
        module_name += '/';
    }
    m_ctx.modules.insert(module_name);
    return std::make_shared<Import>(import_token.location(), module_name);
}

/*
 * Precedence climbing method (https://en.wikipedia.org/wiki/Operator-precedence_parser):
 *
 * parse_expression()
 *    return parse_expression_1(parse_primary(), 0)
 *
 * parse_expression_1(lhs, min_precedence)
 *    lookahead := peek next token
 *    while lookahead is a binary operator whose precedence is >= min_precedence
 *      *op := lookahead
 *      advance to next token
 *      rhs := parse_primary ()
 *      lookahead := peek next token
 *      while lookahead is a binary operator whose precedence is greater
 *              than op's, or a right-associative operator
 *              whose precedence is equal to op's
 *        rhs := parse_expression_1 (rhs, precedence of op + 1)
 *        lookahead := peek next token
 *      lhs := the result of applying op with operands lhs and rhs
 *    return lhs
 */
std::shared_ptr<Expression> Parser::parse_expression()
{
    auto primary = parse_primary_expression();
    if (!primary)
        return nullptr;
    return parse_expression_1(primary, 0);
}

/*
 * Precedences according to https://en.cppreference.com/w/c/language/operator_precedence:
 */

template<TokenCode Count>
class OperatorDefs {
public:
    OperatorDefs()
    {
        for (auto ix = 0u; ix < (int) Count; ++ix) {
            defs_by_code[ix] = { (TokenCode) ix, OperandKind::None, OperandKind::None, -1, OperandKind::None, -1 };
        }
        for (auto const& def : operators) {
            if (def.op == TokenCode::Unknown)
                break;
            defs_by_code[(int)def.op] = { def.op, def.lhs_kind, def.rhs_kind, def.precedence, def.unary_kind, def.unary_precedence };
        }
    }

    [[nodiscard]] OperatorDef const& find(TokenCode code) const
    {
        int c = static_cast<int>(code);
        if (code >= TokenCode::count) {
            return defs_by_code[0];
        }
        assert(defs_by_code[c].op == code);
        return defs_by_code[c];
    }

    [[nodiscard]] bool is_binary(TokenCode code) const
    {
        return find(code).lhs_kind != OperandKind::None;
    }

    [[nodiscard]] bool is_unary(TokenCode code) const
    {
        return find(code).unary_kind != OperandKind::None;
    }

    [[nodiscard]] int binary_precedence(TokenCode code) const
    {
        return find(code).precedence;
    }

    [[nodiscard]] int unary_precedence(TokenCode code) const
    {
        return find(code).unary_precedence;
    }

    Associativity associativity(TokenCode code)
    {
        switch (code) {
        case TokenCode::Equals:
        case Scribble::KeywordIncEquals:
        case Scribble::KeywordDecEquals:
            return Associativity::RightToLeft;
        default:
            return Associativity::LeftToRight;
        }
    }

private:
    OperatorDef operators[(int)Count] = {
        { TokenCode::Equals, OperandKind::Value, OperandKind::Value, 1 },
        { Scribble::KeywordIncEquals, OperandKind::Value, OperandKind::Value, 1 },
        { Scribble::KeywordDecEquals, OperandKind::Value, OperandKind::Value, 1 },
        { TokenCode::LogicalOr, OperandKind::Value, OperandKind::Value, 3 },
        { TokenCode::LogicalAnd, OperandKind::Value, OperandKind::Value, 4 },
        { TokenCode::Pipe, OperandKind::Value, OperandKind::Value, 5 },
        { TokenCode::Hat, OperandKind::Value, OperandKind::Value, 6 },
        { TokenCode::Ampersand, OperandKind::Value, OperandKind::Value, 7 },
        { TokenCode::EqualsTo, OperandKind::Value, OperandKind::Value, 8 },
        { TokenCode::NotEqualTo, OperandKind::Value, OperandKind::Value, 8 },
        { Scribble::KeywordRange, OperandKind::Value, OperandKind::Value, 8 },
        { TokenCode::GreaterThan, OperandKind::Value, OperandKind::Value, 9 },
        { TokenCode::LessThan, OperandKind::Value, OperandKind::Value, 9 },
        { TokenCode::GreaterEqualThan, OperandKind::Value, OperandKind::Value, 9 },
        { TokenCode::LessEqualThan, OperandKind::Value, OperandKind::Value, 9 },
        { TokenCode::ShiftLeft, OperandKind::Value, OperandKind::Value, 10 },
        { TokenCode::ShiftRight, OperandKind::Value, OperandKind::Value, 10 },
        { TokenCode::Plus, OperandKind::Value, OperandKind::Value, 11, OperandKind::Value, 13 },
        { TokenCode::Minus, OperandKind::Value, OperandKind::Value, 11, OperandKind::Value, 13 },
        { TokenCode::Asterisk, OperandKind::Value, OperandKind::Value, 12, OperandKind::Value, 13 },
        { TokenCode::Slash, OperandKind::Value, OperandKind::Value, 12 },
        { TokenCode::Percent, OperandKind::Value, OperandKind::Value, 12 },
        { TokenCode::Tilde, OperandKind::None, OperandKind::None, -1, OperandKind::Value, 13 },
        { TokenCode::ExclamationPoint, OperandKind::None, OperandKind::None, -1, OperandKind::Value, 13 },
        { TokenCode::AtSign, OperandKind::None, OperandKind::None, -1, OperandKind::Value, 13 },
        { TokenCode::Period, OperandKind::Value, OperandKind::Value, 14, OperandKind::Value, 14 },
        { TokenCode::OpenBracket, OperandKind::Value, OperandKind::Value, 14 },
        { TokenCode::OpenParen, OperandKind::Value, OperandKind::Value, 14 },
        { TokenCode::CloseBracket, OperandKind::Value, OperandKind::Value, -1 },
        { TokenCode::Unknown, OperandKind::None, OperandKind::None, -1 },
    };

    OperatorDef defs_by_code[(int)Count] = {
        { TokenCode::Unknown, OperandKind::None, OperandKind::None, -1, OperandKind::None, -1 }
    };
};

static OperatorDefs<TokenCode::count> operator_defs;

std::shared_ptr<Expression> Parser::parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence)
{
    while (operator_defs.is_binary(m_lexer.current_code()) && operator_defs.binary_precedence(m_lexer.current_code()) >= min_precedence) {
        auto op = m_lexer.lex();
        std::shared_ptr<Expression> rhs;
        if (operator_defs.associativity(op.code()) == Associativity::LeftToRight) {
            auto open_bracket = op.code() == TokenCode::OpenBracket;
            switch (op.code()) {
            case TokenCode::OpenParen: {
                Expressions expressions;
                if (m_lexer.current_code() != TokenCode::CloseParen) {
                    while (true) {
                        auto expr = parse_expression();
                        if (expr == nullptr)
                            return nullptr;
                        expressions.push_back(expr);
                        if (m_lexer.current_code() == TokenCode::CloseParen)
                            break;
                        if (!m_lexer.expect(TokenCode::Comma))
                            return nullptr;
                    }
                }
                m_lexer.lex();
                rhs = std::make_shared<ExpressionList>(op.location(), expressions);
                break;
            }
            default: {
                rhs = parse_primary_expression();
                if (!rhs)
                    return nullptr;
                while ((open_bracket && (m_lexer.current_code() != TokenCode::CloseBracket)) || (operator_defs.binary_precedence(m_lexer.current_code()) > operator_defs.binary_precedence(op.code())))
                    rhs = parse_expression_1(rhs, (open_bracket) ? 0 : (operator_defs.binary_precedence(op.code()) + 1));
                break;
            }
            }
            if (open_bracket) {
                if (!m_lexer.expect(TokenCode::CloseBracket))
                    return nullptr;
            }
        } else {
            rhs = parse_expression();
        }
        lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
    }

    // Pull up unary expressions with lower precedence than the binary we just parsed.
    // This is for cases like @var.error.
    if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(lhs); binary != nullptr) {
        if (auto lhs_unary = std::dynamic_pointer_cast<UnaryExpression>(binary->lhs()); lhs_unary != nullptr && operator_defs.unary_precedence(lhs_unary->op().code()) < operator_defs.binary_precedence(binary->op().code())) {
            auto pushed_down = std::make_shared<BinaryExpression>(lhs_unary->operand(), binary->op(), binary->rhs());
            lhs = std::make_shared<UnaryExpression>(lhs_unary->op(), pushed_down);
        }
    }
    return lhs;
}

std::shared_ptr<Expression> Parser::parse_primary_expression()
{
    std::shared_ptr<Expression> expr { nullptr };
    auto t = m_lexer.lex();
    switch (t.code()) {
    case TokenCode::OpenParen: {
        expr = parse_expression();
        if (!m_lexer.expect(TokenCode::CloseParen)) {
            return nullptr;
        }
        break;
    }
    case TokenCode::Integer:
    case TokenCode::HexNumber: {
        expr = std::make_shared<IntLiteral>(t);
        break;
    }
    case TokenCode::Float:
        expr = std::make_shared<FloatLiteral>(t);
        break;
    case TokenCode::DoubleQuotedString:
        expr = std::make_shared<StringLiteral>(t);
        break;
    case TokenCode::SingleQuotedString:
        if (t.value().length() != 1) {
            m_lexer.add_error(t, "Syntax Error: Single-quoted string should only hold a single character, not '{}'", t.value());
            return nullptr;
        }
        expr = std::make_shared<CharLiteral>(t);
        break;
    case Scribble::KeywordTrue:
    case Scribble::KeywordFalse:
        expr = std::make_shared<BooleanLiteral>(t);
        break;
    case TokenCode::Identifier:
        expr = std::make_shared<Variable>(t.location(), t.to_string());
        break;
    default:
        if (operator_defs.is_unary(t.code())) {
            auto operand = parse_primary_expression();
            if (!operand)
                return nullptr;
            expr = std::make_shared<UnaryExpression>(t, operand);
            break;
        }
        m_lexer.add_error(t, "Syntax Error: Expected literal or variable, got '{}' ({})", t.value(), t.code_name());
        return nullptr;
    }
    return expr;
}

}
