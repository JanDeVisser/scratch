/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <SDL2_gfxPrimitives.h>
#include <SDL_image.h>

#include <core/Logging.h>

#include "Widget/SDLContext.h"
#include <App/Scratch.h>

#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 1024
#endif

#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 768
#endif

namespace Scratch {

using namespace Obelix;

logging_category(scratch);

Scratch::ScratchCommands::ScratchCommands()
{
    register_command({ "enlarge-font", "Enlarge editor font", {},
        [](Widget&, strings const&) -> void {
            App::instance().enlarge_font();
        }
    }, { SDLK_EQUALS, KMOD_GUI } );

    register_command({ "invoke", "Invoke command",
        {
            { "Command", CommandParameterType::Command }
        },
        [](Widget& app, strings const& args) -> void {
             if (auto scheduled_command = app.command(args[0]); scheduled_command.has_value()) {
                 App::instance().schedule(scheduled_command.value());
             }
        }
    }, { SDLK_x, KMOD_GUI });

    register_command({ "reset-font", "Reset editor font", {},
        [](Widget&, strings const&) -> void {
            auto& app = App::instance();
            app.reset_font();
        }
    }, { SDLK_0, KMOD_GUI });

    register_command({ "scratch-quit", "Quits the editor", {},
        [](Widget&, strings const&) -> void {
            App::instance().quit();
        }
    }, { SDLK_q, KMOD_CTRL });

    register_command({ "set-fixed-width-font", "Set fixed width (editor) font",
        {
            {
                "Font file name", CommandParameterType::ExistingFilename
            }
        },
        [](Widget&, strings const& args) -> void {
            Scratch::instance().set_font(args[0]);
        }
    });

    register_command({ "shrink-font", "Shrink editor font", {},
        [](Widget&, strings const&) -> void {
            App::instance().shrink_font();
        }
    }, { SDLK_MINUS, KMOD_GUI });

}

Scratch::ScratchCommands Scratch::s_scratch_commands;

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
    auto icon_surface = IMG_LoadTyped_RW(SDL_RWFromFile("scratch.png", "rb"), 1, "PN");
    if(!icon_surface) {
        log_error("Could not load application icon");
        return;
    }
    SDL_SetWindowIcon(context()->window(), icon_surface);
    SDL_FreeSurface(icon_surface);
    m_commands = &s_scratch_commands;
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
    app.add_status_bar_applet(7, [](WindowedWidget* applet) -> void {
        applet->render_fixed(5, 2, App::instance().last_key().to_string(), SDL_Color { 0xff, 0xff, 0xff, 0xff });
    });
    app.add_status_bar_applet(5, [](WindowedWidget* applet) -> void {
        PaletteIndex box_color;
        auto f = App::instance().fps();
        if (f >= 55) {
            box_color = PaletteIndex::ANSIGreen;
        } else if (f >= 40) {
            box_color = PaletteIndex::ANSIYellow;
        } else {
            box_color = PaletteIndex::ANSIBrightRed;
        }
        applet->box(SDL_Rect { 0, 0, 0, 0 }, App::instance().color(box_color));
        applet->render_fixed_centered(2, "fps", SDL_Color { 0xff, 0xff, 0xff, 0xff });
    });
    app.add_status_bar_applet(7, [](WindowedWidget* applet) -> void {
        PaletteIndex box_color;
        auto *doc = Scratch::scratch().editor()->document();
        if (doc == nullptr)
            return;
        auto t = doc->last_parse_time();
        if (t < 10) {
            box_color = PaletteIndex::ANSIGreen;
        } else if (t < 20) {
            box_color = PaletteIndex::ANSIYellow;
        } else {
            box_color = PaletteIndex::ANSIBrightRed;
        }
        applet->box(SDL_Rect { 0, 0, 0, 0 }, App::instance().color(box_color));
        applet->render_fixed_centered(2, "parse", SDL_Color { 0xff, 0xff, 0xff, 0xff });
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
