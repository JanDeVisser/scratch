/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <App/Document.h>
#include <App/Scratch.h>
#include <Scribble/Interp/ExpressionResult.h>
#include <Scribble/Interp/Interpreter.h>
#include <Scribble/Scribble.h>
#include <Scribble/Parser.h>
#include <Scribble/Syntax/Statement.h>
#include <Widget/Alert.h>

namespace Scratch::Scribble {

using namespace Scratch::Interp;

ScribbleCommands::ScribbleCommands()
{
    register_command(
        { "evaluate-buffer", "Evaluates the script in the current buffer", {},
            [](Widget& w, strings const&) -> void {
                auto* doc = Scratch::editor()->document();
                auto& text = doc->text();
                auto project_maybe = compile_project(doc->path(), std::make_shared<StringBuffer>(text));
                if (project_maybe.is_error()) {
                    doc->bottom(false);
                    doc->insert("\n// " + project_maybe.error().to_string());
                    return;
                }
                auto result = interpret(std::dynamic_pointer_cast<Project>(project_maybe.value()));
                if (result.is_error()) {
                    doc->bottom(false);
                    doc->insert("\n// " + result.error().to_string());
                    return;
                }
                auto r = std::dynamic_pointer_cast<Interp::ExpressionResult>(result.value());
                doc->bottom(false);
                doc->insert("\n// " + r->value().to_string());
            } },
        { SDLK_e, KMOD_CTRL });
}

ScribbleCommands Scribble::s_scribble_commands;

Scribble::Scribble(bool ignore_ws)
    : ScratchParser()
    , m_ignore_ws(ignore_ws)
{
    lexer().add_scanner<Obelix::QStringScanner>("\"'", true);
    IdentifierScanner::Config ident_config;
    ident_config.filter = "X_-";
    lexer().add_scanner<Obelix::IdentifierScanner>(ident_config);
    lexer().add_scanner<Obelix::NumberScanner>(Obelix::NumberScanner::Config { true, false, true, false, true });
    lexer().add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { m_ignore_ws, m_ignore_ws, m_ignore_ws });
    lexer().add_scanner<Obelix::CommentScanner>(true,
        Obelix::CommentScanner::CommentMarker { false, false, "/*", "*/" },
        Obelix::CommentScanner::CommentMarker { false, true, "//", "" });
    lexer().add_scanner<Obelix::KeywordScanner>(
        KeywordBreak, "break",
        KeywordCase, "case",
        KeywordCmd, "cmd",
        KeywordConst, "const",
        KeywordContinue, "continue",
        KeywordDecEquals, "-=",
        KeywordDefault, "default",
        KeywordElse, "else",
        KeywordElif, "elif",
        KeywordFor, "for",
        KeywordFunc, "func",
        KeywordIf, "if",
        KeywordImport, "import",
        KeywordIn, "in",
        KeywordIncEquals, "+=",
        KeywordIntrinsic, "intrinsic",
        KeywordLink, "->",
        KeywordRange, "..",
        KeywordReturn, "return",
        KeywordSwitch, "switch",
        KeywordVar, "var",
        KeywordWhile, "while",

        KeywordTrue, "true",
        KeywordFalse, "false");
}

DisplayToken Scribble::colorize(TokenCode code, std::string_view const& text)
{
    return token_for(code, text);
}

Token const& Scribble::next_token()
{
    return lex();
}

std::optional<ScheduledCommand> Scribble::command(std::string const& name) const
{
    if (auto* cmd = s_scribble_commands.get(name); cmd != nullptr)
        return ScheduledCommand { dynamic_cast<Widget&>(*Scratch::editor()->buffer()), *cmd };
    return {};
}

std::vector<Command> Scribble::commands() const
{
    return *s_scribble_commands;
}

DisplayToken token_for(TokenCode code, std::string_view const& text)
{
    PaletteIndex color;
    if (code >= TokenCode::Keyword0 && code <= TokenCode::Keyword30) {
        color = PaletteIndex::Keyword;
    } else {
        switch (code) {
        case TokenCode::Comment:
            color = PaletteIndex::Comment;
            break;
        case TokenCode::Identifier:
            color = PaletteIndex::Identifier;
            break;
        case TokenCode::DoubleQuotedString:
            color = PaletteIndex::CharLiteral;
            break;
        case TokenCode::SingleQuotedString:
            color = PaletteIndex::String;
            break;
        case Scribble::KeywordTrue:
        case Scribble::KeywordFalse:
        case TokenCode::Integer:
        case TokenCode::Float:
            color = PaletteIndex::Number;
            break;
        default:
            color = PaletteIndex::Punctuation;
            break;
        }
    }
    return DisplayToken { text, color };
}

} // Scratch
