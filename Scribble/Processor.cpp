/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>

#include <Scribble/Processor.h>
#include <Scribble/Syntax/Statement.h>
#include <Scribble/Syntax/Syntax.h>
#include <Scribble/Parser.h>

namespace Scratch::Scribble {

using namespace Obelix;

logging_category(processor);

std::string sanitize_module_name(std::string const& unsanitized)
{
    auto ret = to_lower(unsanitized);
    if (ret.ends_with(".scb"))
        ret = ret.substr(0, ret.length() - 4);
    if (ret.starts_with("./"))
        ret = ret.substr(2);
    return ret;
}

ProcessResult parse(ParserContext& ctx, std::string const& module_name)
{
    auto parser_or_error = Parser::create(ctx, module_name);
    if (parser_or_error.is_error()) {
        return SyntaxError { {}, parser_or_error.error().message() };
    }
    auto parser = parser_or_error.value();
    ProcessResult ret;
    ret = parser->parse();
    for (auto const& e : parser->lexer().errors())
        ret.error(e);
    return ret;
}

#define PROCESS_RESULT(res)        \
    ({                             \
        auto __result = res;       \
        if (result.is_error()) { \
            return __result;       \
        }                          \
    })

#define TRY_PROCESS_RESULT(tree, ctx)               \
    ({                                              \
        process(tree, ctx, result);                 \
        if (__result.is_error()) {                  \
            return __result;                        \
        }                                           \
    })

ProcessResult compile_project(std::string const& name)
{
    ParserContext ctx;

    ProcessResult result;
    result = std::make_shared<Project>(name);
    process(result.value(), ctx, result);
    if (result.is_error())
        return result;
    if (true)
        std::cout << "\n\nParsed tree:\n" << result.value()->to_xml() << "\n";
    return result;
}

INIT_NODE_PROCESSOR(ParserContext)

NODE_PROCESSOR(Project)
{
    auto compilation = std::dynamic_pointer_cast<Project>(tree);
    Modules modules = compilation->modules();
    auto main_done = std::any_of(modules.begin(), modules.end(), [&compilation](auto const& m) {
        return m->name() == compilation->main_module();
    });
    if (!main_done) {
        auto res = parse(ctx, compilation->main_module());
        result += res;
        if (result.is_error())
            return result.error();
        modules.push_back(std::dynamic_pointer_cast<Module>(res.value()));
        while (!ctx.modules.empty()) {
            Strings module_names;
            for (auto const& m : ctx.modules) {
                module_names.push_back(m);
            }
            ctx.modules.clear();
            for (auto const& m : module_names) {
                res = parse(ctx, m);
                if (res.has_value())
                    modules.push_back(std::dynamic_pointer_cast<Module>(res.value()));
                result += res;
            }
        }
    }
    auto root_done = std::any_of(modules.begin(), modules.end(), [](auto const& m) {
        return m->name() == "/";
    });
    if (!root_done) {
        auto res = parse(ctx, "/");
        if (res.has_value())
            modules.push_back(std::dynamic_pointer_cast<Module>(res.value()));
        result += res;
    }
    return std::make_shared<Project>(modules, compilation->main_module());
}

}
