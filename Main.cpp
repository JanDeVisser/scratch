/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Logging.h>

#include <App.h>
#include <Editor.h>
#include <Menu.h>
#include <MessageBar.h>
#include <StatusBar.h>
#include <Widget.h>

using namespace Obelix;

namespace Scratch {

logging_category(scratch);

struct Config {
public:
    Config() = default;
    Config(int argc, char const** argv);

    std::string filename;

    template <typename T>
    T cmdline_flag(std::string const& flag, T const& default_value = T()) const
    {
        if (m_cmdline_flags.contains(flag)) {
            auto val = m_cmdline_flags.at(flag);
            assert(std::holds_alternative<T>(val));
            return get<T>(val);
        }
        return default_value;
    }

    bool help { false };

private:
    std::unordered_map<std::string, std::variant<std::string,bool>> m_cmdline_flags;
};

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
        }
        if (!strcmp(argv[ix], "--help")) {
            help = true;
        }
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

void run_app(int argc, char const** argv)
{
    Config config(argc, argv);
    debug(scratch, "The logger works!");
    auto display_maybe = Display::create();
    if (display_maybe.is_error()) {
        fprintf(stderr, "ERROR: %s\n", display_maybe.error().c_str());
        exit(-1);
    }
    App app("Scratch", display_maybe.value());

//    auto menu = new MenuBar(MenuDescriptions {
//        { "File", { { "Quit", [&app]() { app.quit(); } } } }
//    });
//    app.add_component(menu);
    auto editor = new Editor();
    editor->document().load("App.cpp");
    app.add_component(editor);
    app.add_component(new StatusBar());
    app.add_component(new MessageBar());
    app.event_loop();
}

}

int main(int argc, char const** argv)
{
    Scratch::run_app(argc, argv);
    return 0;
}
