/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <SDL.h>

#include <Scratch.h>
#include <Commands/Command.h>
#include <Commands/CommandHandler.h>

namespace Scratch {

class ArgumentHandler : public ModalWidget {
protected:
    ArgumentHandler(CommandHandler*, CommandParameter const&, int);

    CommandHandler* m_handler;
    CommandParameter const& m_parameter;
};

class DefaultArgumentHandler : public ArgumentHandler {
public:
    DefaultArgumentHandler(CommandHandler*, CommandParameter const&);
    void render() override;
    bool dispatch(SDL_Keysym sym) override;
    void handle_text_input() override;

private:
    std::string m_value;
    int m_pos { 0 };
};

template<class EntryCls, typename ContextCls>
class ListArgumentHandler;

template<class EntryCls, typename ContextCls = int>
inline std::string line_text(EntryCls const& entry, ContextCls const&)
{
    return entry.to_string();
}

template<class EntryCls, typename ContextCls = int>
inline std::vector<EntryCls> get_entries(CommandParameter const&, ContextCls const&)
{
    fatal("get_entries unimplemented for EntryCls '{}' and ContextCls '{}'",
        typeid(EntryCls).name(), typeid(ContextCls).name());
}

template<class EntryCls, typename ContextCls = int>
inline void submit(CommandHandler* handler, EntryCls const& entry, ContextCls const&)
{
    handler->argument_done(entry.to_string());
}

template<class EntryCls, typename ContextCls = int>
inline bool handle(ListArgumentHandler<EntryCls, ContextCls>*, EntryCls const&, ContextCls const&, SDL_Keysym)
{
    return false;
}

template<typename ContextCls = int>
inline std::string line_text(std::string const& entry, ContextCls const&)
{
    return entry;
}

template<typename ContextCls = int>
inline void submit(CommandHandler* handler, std::string const& entry, ContextCls const&)
{
    handler->argument_done(entry);
}

template<class EntryCls, typename ContextCls = int>
class ListArgumentHandler : public ArgumentHandler {
public:
    ListArgumentHandler(CommandHandler* handler, CommandParameter const& parameter, ContextCls const& ctx)
        : ArgumentHandler(handler, parameter, App::instance().height() * 0.66)
        , m_lines((height() - App::instance().context()->character_height() - 10) / (App::instance().context()->character_height() + 2))
    {
        set_entries(m_current_context);
    }

    void set_entries(ContextCls const& ctx)
    {
        m_current_context = ctx;
        m_entries = std::move(get_entries<EntryCls>(m_parameter, ctx));
        filter_matches();
    }

    [[nodiscard]] CommandParameter const& parameter() const
    {
        return m_parameter;
    }

    void render() override
    {
        box(SDL_Rect { 0, 0, 0, 0 }, SDL_Color { 0x2c, 0x2c, 0x2c, 0xff });
        rectangle(SDL_Rect { 2, 2, width() - 4, height() - 4 }, { 0xff, 0xff, 0xff, 0xff });
        render_fixed(8, 8, m_parameter.prompt);
        auto y = App::instance().context()->character_height() + 10;
        auto count = 0;
        for (auto const& match : m_matches) {
            if (count < m_top) {
                count++;
                continue;
            }
            if (count >= m_top + m_lines) {
                break;
            }
            if (count == m_current) {
                SDL_Rect r {
                    4,
                    y - 1,
                    -4,
                    App::instance().context()->character_height() + 1
                };
                box(r, App::instance().color(PaletteIndex::CurrentLineFill));
                rectangle(r, App::instance().color(PaletteIndex::CurrentLineEdge));
            }
            render_fixed(10, y, match.text);
            y += App::instance().context()->character_height() + 2;
            if (y > height() - App::instance().context()->character_height() - 2)
                break;
            count++;
        }
    }

    bool dispatch(SDL_Keysym sym) override
    {
        if (handle(this, m_matches[m_current].entry, m_current_context, sym))
            return true;
        switch (sym.sym) {
        case SDLK_ESCAPE: {
            m_handler->abort();
            return true;
        }
        case SDLK_RETURN: {
            submit(m_handler, m_matches[m_current].entry, m_current_context);
            return true;
        };
        case SDLK_UP: {
            if (m_current > 0)
                m_current--;
            while (m_current < m_top)
                m_top--;
            return true;
        };
        case SDLK_DOWN: {
            if (m_current < m_matches.size() - 1)
                m_current++;
            while (m_current > m_top + m_lines - 1)
                m_top++;
            return true;
        };
        case SDLK_PAGEUP: {
            if (m_current >= m_lines) {
                m_current -= m_lines;
                if (m_top >= m_lines)
                    m_top -= m_lines;
                else
                    m_top = 0;
            }
            return true;
        };
        case SDLK_PAGEDOWN: {
            if (m_current < m_matches.size() - m_lines) {
                m_current += m_lines;
                m_top += m_lines;
            }
            return true;
        };
        case SDLK_BACKSPACE: {
            if (!m_search_str.empty()) {
                m_search_str.erase(m_search_str.length() - 1, 1);
                filter_matches();
            }
            return true;
        }
        default:
            return false;
        }
    }

    void filter_matches()
    {
        m_matches.clear();
        if (m_search_str.empty()) {
            for (auto const& e : m_entries) {
                m_matches.emplace_back(e, m_current_context);
            }
            return;
        }
        for (auto const& e : m_entries) {
            auto s_ix = 0;
            auto text = line_text(e, m_current_context);
            for (auto ix = 0; ix < text.length(); ++ix) {
                if (toupper(m_search_str[s_ix]) == toupper(text[ix])) {
                    if (++s_ix == m_search_str.length()) {
                        break;
                    }
                }
            }
            if (s_ix == m_search_str.length()) {
                m_matches.emplace_back(e, m_current_context);
            }
        }
    }

    void handle_text_input() override
    {
        m_search_str += App::instance().input_buffer();
        filter_matches();
    }

private:
    struct Match {
        EntryCls entry;
        std::string text;

        explicit Match(EntryCls e, ContextCls const& ctx)
            : entry(std::move(e))
            , text(line_text(entry, ctx))
        {
        }
    };

    std::vector<EntryCls> m_entries;
    ContextCls m_current_context;
    int m_lines;
    int m_top { 0 };
    int m_current { 0 };
    std::vector<Match> m_matches;
    std::string m_search_str;
};

ModalWidget* create_argument_handler(CommandHandler*, CommandParameter const&);

}
