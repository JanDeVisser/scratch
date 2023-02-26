/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Commands/ArgumentHandler.h>

namespace Scratch {

DefaultArgumentHandler::DefaultArgumentHandler(CommandHandler* handler, CommandParameter const& parameter)
    : ArgumentHandler(handler, parameter, 2 * (App::instance().context()->character_height() + 4) + 12)
{
    if (parameter.get_default != nullptr) {
        m_value = parameter.get_default();
        m_pos = static_cast<int>(m_value.length());
    }
}

void DefaultArgumentHandler::render()
{
    box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
    rectangle(SDL_Rect { 2, 2, width() - 4, height() - 4 }, { 0xff, 0xff, 0xff, 0xff });
    render_fixed(8, 4, m_parameter.prompt);
    render_fixed(8, App::instance().context()->character_height() + 12, m_value);

    static auto time_start = std::chrono::system_clock::now();
    auto time_end = std::chrono::system_clock::now();
    const long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
    if (elapsed > 400) {
        SDL_Rect r {
            8 + App::instance().context()->character_width() * m_pos,
            App::instance().context()->character_height() + 12,
            1,
            App::instance().context()->character_height()
        };
        box(r, App::instance().color(PaletteIndex::Cursor));
        if (elapsed > 800)
            time_start = time_end;
    }
}

bool DefaultArgumentHandler::dispatch(SDL_Keysym sym)
{
    switch (sym.sym) {
    case SDLK_ESCAPE: {
        m_handler->abort();
        return true;
    }
    case SDLK_RETURN: {
        m_handler->argument_done(m_value);
        return true;
    };
    case SDLK_LEFT: {
        if (m_pos > 0)
            m_pos--;
        return true;
    }
    case SDLK_RIGHT: {
        if (m_pos < static_cast<int>(m_value.length()))
            m_pos++;
        return true;
    }
    case SDLK_HOME:
        m_pos = 0;
        return true;
    case SDLK_END:
        m_pos = static_cast<int>(m_value.length());
        return true;
    case SDLK_BACKSPACE: {
        if (m_pos > 0) {
            m_value.erase(m_pos - 1, 1);
            m_pos--;
        }
        return true;
    }
    case SDLK_UP: {
        if (m_parameter.type == CommandParameterType::Integer) {
            auto val = to_long(m_value);
            ++val;
            m_value = format("{}", val);
            m_pos = static_cast<int>(m_value.length());
        }
        return true;
    };
    case SDLK_DOWN: {
        if (m_parameter.type == CommandParameterType::Integer) {
            if (auto val = to_long(m_value); val > 0) {
                --val;
                m_value = format("{}", val);
                m_pos = static_cast<int>(m_value.length());
            }
        }
        return true;
    };
    default:
        return false;
    }
}

void DefaultArgumentHandler::handle_text_input()
{
    auto str = App::instance().input_buffer();
    if (str.empty())
        return;
    if (m_pos <= static_cast<int>(m_value.length())) {
        switch (m_parameter.type) {
        case CommandParameterType::String: {
            m_value.insert(m_pos, str);
            m_pos += static_cast<int>(str.length());
        } break;
        case CommandParameterType::Integer: {
            for (auto const& ch : str) {
                if (isdigit(ch)) {
                    m_value.insert(m_pos, 1, ch);
                    ++m_pos;
                }
            }
        } break;
        default:
            fatal("Unreachable");
        }
    }
}

}
