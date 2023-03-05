/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <core/Format.h>
#include <core/Logging.h>

using namespace Obelix;

namespace Scratch::Scribble {

#define ENUMERATE_SYNTAXNODETYPES(S)    \
    S(SyntaxNode)                       \
    S(Statement)                        \
    S(Block)                            \
    S(FunctionBlock)                    \
    S(Project)                          \
    S(Module)                           \
    S(ExpressionType)                   \
    S(StringTemplateArgument)           \
    S(IntegerTemplateArgument)          \
    S(Expression)                       \
    S(ExpressionList)                   \
    S(EnumValue)                        \
    S(EnumDef)                          \
    S(TypeDef)                          \
    S(IntLiteral)                       \
    S(CharLiteral)                      \
    S(FloatLiteral)                     \
    S(StringLiteral)                    \
    S(BooleanLiteral)                   \
    S(StructLiteral)                    \
    S(ArrayLiteral)                     \
    S(Identifier)                       \
    S(Variable)                         \
    S(This)                             \
    S(BinaryExpression)                 \
    S(UnaryExpression)                  \
    S(CastExpression)                   \
    S(Assignment)                       \
    S(FunctionCall)                     \
    S(Import)                           \
    S(Pass)                             \
    S(Label)                            \
    S(Goto)                             \
    S(FunctionDecl)                     \
    S(NativeFunctionDecl)               \
    S(IntrinsicDecl)                    \
    S(FunctionDef)                      \
    S(ExpressionStatement)              \
    S(VariableDeclaration)              \
    S(StaticVariableDeclaration)        \
    S(LocalVariableDeclaration)         \
    S(GlobalVariableDeclaration)        \
    S(StructDefinition)                 \
    S(StructForward)                    \
    S(Return)                           \
    S(Break)                            \
    S(Continue)                         \
    S(Branch)                           \
    S(IfStatement)                      \
    S(WhileStatement)                   \
    S(ForStatement)                     \
    S(CaseStatement)                    \
    S(DefaultCase)                      \
    S(SwitchStatement)                  \
    S(ExpressionResult)                 \
    S(ExpressionResultList)             \
    S(StatementExecutionResult)

enum class SyntaxNodeType {
#undef ENUM_SYNTAXNODETYPE
#define ENUM_SYNTAXNODETYPE(type) type,
    ENUMERATE_SYNTAXNODETYPES(ENUM_SYNTAXNODETYPE)
#undef ENUM_SYNTAXNODETYPE
        NodeList,
    Count
};

constexpr char const* SyntaxNodeType_name(SyntaxNodeType type)
{
    switch (type) {
#undef ENUM_SYNTAXNODETYPE
#define ENUM_SYNTAXNODETYPE(type) \
    case SyntaxNodeType::type:    \
        return #type;
        ENUMERATE_SYNTAXNODETYPES(ENUM_SYNTAXNODETYPE)
#undef ENUM_SYNTAXNODETYPE
    case SyntaxNodeType::NodeList:
        return "NodeList";
    default:
        fatal("Unknown SyntaxNodeType value '{}'", (int)type);
    }
}
}

namespace Obelix {

using namespace Scratch::Scribble;

template<>
struct to_string<SyntaxNodeType> {
    std::string operator()(SyntaxNodeType val)
    {
        return SyntaxNodeType_name(val);
    }
};

template<>
struct try_to_double<SyntaxNodeType> {
    std::optional<double> operator()(SyntaxNodeType val)
    {
        return static_cast<double>(val);
    }
};

template<>
struct try_to_long<SyntaxNodeType> {
    std::optional<long> operator()(SyntaxNodeType val)
    {
        return static_cast<long>(val);
    }
};

}
