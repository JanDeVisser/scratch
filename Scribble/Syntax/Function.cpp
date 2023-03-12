/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Scribble/Syntax/Function.h>

namespace Scratch::Scribble {

extern_logging_category(scribble);

// -- FunctionDecl ----------------------------------------------------------

FunctionDecl::FunctionDecl(Span location, std::string module, std::shared_ptr<Identifier> identifier, Identifiers parameters)
    : Statement(location)
    , m_module(std::move(module))
    , m_identifier(std::move(identifier))
    , m_parameters(std::move(parameters))
{
}

std::string const& FunctionDecl::module() const
{
    return m_module;
}

std::shared_ptr<Identifier> const& FunctionDecl::identifier() const
{
    return m_identifier;
}

std::string const& FunctionDecl::name() const
{
    return identifier()->name();
}

Identifiers const& FunctionDecl::parameters() const
{
    return m_parameters;
}

std::string FunctionDecl::attributes() const
{
    return format(R"(name="{}")", name());
}

Nodes FunctionDecl::children() const
{
    Nodes ret;
    for (auto& param : m_parameters) {
        ret.push_back(param);
    }
    return ret;
}

std::string FunctionDecl::to_string() const
{
    return format("func {}({})", name(), parameters_to_string());
}

std::string FunctionDecl::parameters_to_string() const
{
    return join(m_parameters, ", ", [](auto const& param) { return param->name(); });
}

bool FunctionDecl::is_complete() const
{
    return m_identifier->is_complete() &&
        std::all_of(m_parameters.begin(), m_parameters.end(), [](auto const& param) -> bool { return param->is_complete(); });
}

// -- NativeFunctionDecl ----------------------------------------------------

NativeFunctionDecl::NativeFunctionDecl(Span location, std::string module, std::shared_ptr<Identifier> identifier, Identifiers parameters, std::string native_function)
    : FunctionDecl(location, std::move(module), std::move(identifier), std::move(parameters))
    , m_native_function_name(std::move(native_function))
{
}

std::string const& NativeFunctionDecl::native_function_name() const
{
    return m_native_function_name;
}

std::string NativeFunctionDecl::attributes() const
{
    return format("{} native_function=\"{}\"", FunctionDecl::attributes(), native_function_name());
}

std::string NativeFunctionDecl::to_string() const
{
    return format("{} -> \"{}\"", FunctionDecl::to_string(), native_function_name());
}

bool NativeFunctionDecl::is_complete() const
{
    return FunctionDecl::is_complete() && !native_function_name().empty();
}

// -- IntrinsicDecl ---------------------------------------------------------

IntrinsicDecl::IntrinsicDecl(Span location, std::string module, std::shared_ptr<Identifier> identifier, Identifiers parameters)
    : FunctionDecl(location, std::move(module), std::move(identifier), std::move(parameters))
{
}

std::string IntrinsicDecl::to_string() const
{
    return format("intrinsic {}({})", name(), parameters_to_string());
}

// -- FunctionDef -----------------------------------------------------------

FunctionDef::FunctionDef(Span location, std::shared_ptr<FunctionDecl> func_decl, std::shared_ptr<Statement> statement)
    : Statement(location)
    , m_function_decl(std::move(func_decl))
    , m_statement(std::move(statement))
{
}

std::shared_ptr<FunctionDecl> const& FunctionDef::declaration() const
{
    return m_function_decl;
}

std::shared_ptr<Identifier> const& FunctionDef::identifier() const
{
    return m_function_decl->identifier();
}

std::string const& FunctionDef::name() const
{
    return identifier()->name();
}

Identifiers const& FunctionDef::parameters() const
{
    return m_function_decl->parameters();
}

std::shared_ptr<Statement> const& FunctionDef::statement() const
{
    return m_statement;
}

Nodes FunctionDef::children() const
{
    Nodes ret;
    ret.push_back(m_function_decl);
    if (m_statement) {
        ret.push_back(m_statement);
    }
    return ret;
}

std::string FunctionDef::to_string() const
{
    auto ret = m_function_decl->to_string();
    if (m_statement) {
        ret += " ";
        ret += m_statement->to_string();
    }
    return ret;
}

bool FunctionDef::is_complete() const
{
    return declaration() != nullptr && declaration()->is_complete() && m_statement != nullptr && m_statement->is_complete();
}

}
