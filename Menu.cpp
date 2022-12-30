/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Menu.h>
#include <MessageBar.h>

namespace Scratch {

MenuEntry::MenuEntry(std::string label, Handler handler)
    : m_label(std::move(label))
    , m_handler(std::move(handler))
{
}

std::string const& MenuEntry::label() const
{
    return m_label;
}

void MenuEntry::run()
{
    m_handler();
}

Menu::Menu(int column, MenuDescription const& description)
    : m_column(column)
    , m_title(description.title)
    , m_entries(description.entries)
{
    int width = 0;
    for (auto const& entry : m_entries) {
        if (width < entry.label().length())
            width = static_cast<int>(entry.label().length());
    }
}

std::string const& Menu::title() const
{
    return m_title;
}

MenuEntries const& Menu::entries() const
{
    return m_entries;
}

int Menu::selected() const
{
    return m_selected;
}

void Menu::render() const
{
#if 0
    box(m_window, 0, 0);
    int ix = 0;
    for (auto const& entry : entries()) {
        if (selected() == ix) {
            wattron(m_window, A_REVERSE);
            mvwprintw(m_window, ix + 1, 1, "%s", entry.label().c_str());
            wattroff(m_window, A_REVERSE);
        } else {
            mvwprintw(m_window, ix + 1, 1, "%s", entry.label().c_str());
        }
        ix++;
    }
    wrefresh(m_window);
#endif
}

void Menu::up()
{
    if (--m_selected < 0)
        m_selected = static_cast<int>(m_entries.size()) - 1;
}

void Menu::down()
{
    m_selected = (m_selected + 1) % static_cast<int>(m_entries.size());
}

void Menu::enter()
{
    m_entries[m_selected].run();
}

MenuBar::MenuBar(MenuDescriptions const& description)
    : WindowedWidget(0, 0, 1, App::instance().columns())
{
    m_bar_width = 0;
    for (auto const& menu_description : description) {
        m_entries.emplace_back(std::make_shared<Menu>(m_bar_width + 1, menu_description));
        m_bar_width += static_cast<int>(menu_description.title.length()) + 2;
    }
}

bool MenuBar::active() const
{
    return m_active;
}

void MenuBar::left()
{
    if (active()) {
        if (--m_selected < 0)
            m_selected = static_cast<int>(m_entries.size()) - 1;
    }
}

void MenuBar::right()
{
    if (active() && (m_selected < (m_entries.size() - 1)))
        m_selected = (m_selected + 1) % static_cast<int>(m_entries.size());
}

void MenuBar::up()
{
    if (active())
        m_entries[m_selected]->up();
}

void MenuBar::down()
{
    if (active())
        m_entries[m_selected]->down();
}

void MenuBar::enter()
{
    if (active())
        m_entries[m_selected]->enter();
}

[[nodiscard]] bool MenuBar::handle(KeyCode key)
{
    bool handled { false };
    if (!active()) {
        if (key == KEY_F10) {
            m_active = handled = true;
        }
        return handled;
    }

    switch (key) {
    case KEY_UP:
        up();
        break;
    case KEY_DOWN:
        down();
        break;
    case KEY_LEFT:
        left();
        break;
    case KEY_RIGHT:
        right();
        break;
    case KEY_F10:
        m_active = false;
        break;
    case 13:
        enter();
        break;
    default:
        break;
    }
    // The menubar is modal, so it should swallow all keys when it's active:
    return true;
}

[[nodiscard]] size_t MenuBar::selected() const
{
    return m_selected;
}

void MenuBar::render()
{
#if 0
    App::instance().log("MenuBar::render");
    wattron(window(), A_REVERSE);
    int ix = 0;
    wmove(window(), 0, 0);
    for (auto const& menu : m_entries) {
        if (active() && selected() == ix++) {
            wattroff(window(), A_REVERSE);
            menu->render();
        }
        wprintw(window(), "  %s", menu->title().c_str());
        wattron(window(), A_REVERSE);
    }
    wprintw(window(), "%-*.*s", width() - m_bar_width, width() - m_bar_width, " ");
    wattroff(window(), A_REVERSE);
    wrefresh(window());
#endif
}

}
