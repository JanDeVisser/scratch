/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <SDL2_gfxPrimitives.h>

#include <core/Logging.h>

#include <SDLContext.h>
#include <Scratch.h>

#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 800
#endif

#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 600
#endif

using namespace Obelix;

namespace Scratch {

logging_category(scratch);

Config::Config(int argc, char const** argv)
{
    for (int ix = 1; ix < argc; ++ix) {
        if ((strlen(argv[ix]) > 2) && !strncmp(argv[ix], "--", 2)) {
            if (auto eq_ptr = strchr(argv[ix], '='); eq_ptr == nullptr) {
                m_cmdline_flags[std::string(argv[ix] + 2)] = true;
            } else {
                auto ptr = argv[ix];
                ptr += 2;
                std::string flag = argv[ix] + 2;
                m_cmdline_flags[flag.substr(0, eq_ptr - ptr)] = std::string(eq_ptr + 1);
            }
            continue;
        }
        filename = argv[ix];
    }
    bool enable_log = cmdline_flag<bool>("debug", false);
    auto logfile = cmdline_flag<std::string>("log");
    if (!logfile.empty()) {
        Obelix::Logger::get_logger().set_file(logfile);
        enable_log = true;
    }
    if (enable_log)
        Obelix::Logger::get_logger().enable("scratch");
}

Scratch::Scratch(Config& config, SDLContext *ctx)
    : App("Scratch", ctx)
    , m_config(config)
{
}

Editor* Scratch::editor()
{
    return scratch().m_editor;
}

StatusBar* Scratch::status_bar()
{
    return scratch().m_status_bar;
}

void Scratch::add_status_bar_applet(int size, Renderer renderer)
{
    status_bar()->add_applet(size, std::move(renderer));
}

Scratch& Scratch::scratch()
{
    return dynamic_cast<Scratch&>(App::instance());
}

void Scratch::run_app(int argc, char const** argv)
{
    Config config(argc, argv);
    debug(scratch, "The logger works!");

    auto ctx = new SDLContext(WINDOW_WIDTH, WINDOW_HEIGHT);
    Scratch app(config, ctx);
    auto main_area = new Layout(ContainerOrientation::Horizontal);
    app.add_component(main_area);
    app.add_component(app.m_status_bar = new StatusBar());
    app.add_status_bar_applet(160, [&app](WindowedWidget* applet) -> void {
        applet->render_fixed(10, 2, format("{>3} fps", app.fps()), SDL_Color { 0xff, 0xff, 0xff, 0xff });
    });
    main_area->add_component(app.m_gutter = new Gutter());
    main_area->add_component(app.m_editor = new Editor());
    if (!app.m_config.filename.empty())
        app.m_editor->open_file(app.m_config.filename);
    app.focus(app.m_editor);
    app.event_loop();
}

}

int main(int argc, char const** argv)
{
    Scratch::Scratch::run_app(argc, argv);
    return 0;
}
