/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>

#include <core/FileBuffer.h>

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
    ProcessResult ret;
    auto parser = Parser(ctx);
    if (auto loaded_maybe = parser.read_file(module_name); loaded_maybe.is_error()) {
        ret = SyntaxError { {}, loaded_maybe.error().code() };
        return ret;
    }
    ret = parser.parse();
    for (auto const& e : parser.lexer().errors())
        ret.error(e);
    return ret;
}

ProcessResult parse(ParserContext& ctx, std::shared_ptr<StringBuffer> buffer)
{
    ProcessResult ret;
    auto parser = Parser(ctx);
    parser.assign(std::move(buffer));
    ret = parser.parse(true);
    for (auto const& e : parser.lexer().errors())
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
    auto buffer = FileBuffer::from_file(name);
    if (buffer.is_error()) {
        result = SyntaxError { {}, buffer.error().code() };
        return result;
    }
    result = std::make_shared<Project>(name, buffer.value());
    process(result.value(), ctx, result);
    return result;
}

ProcessResult compile_project(std::string const& name, std::shared_ptr<StringBuffer> buffer)
{
    ParserContext ctx;

    ProcessResult result;
    result = std::make_shared<Project>(name, buffer);
    process(result.value(), ctx, result);
    return result;
}

INIT_NODE_PROCESSOR(ParserContext)

NODE_PROCESSOR(Project)
{
    auto project = std::dynamic_pointer_cast<Project>(tree);
    Modules modules = project->modules();
    auto main_done = std::any_of(modules.begin(), modules.end(), [&project](auto const& m) {
        return m->name() == project->main_module();
    });
    if (!main_done) {
        auto res = parse(ctx, project->main_buffer());
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
    return std::make_shared<Project>(modules, project->main_module(), project->main_buffer());
}

}
