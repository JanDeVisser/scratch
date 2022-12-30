/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <App.h>
#include <Widget.h>

namespace Scratch {

using Handler = std::function<void()>;

class MenuEntry {
public:
    MenuEntry(std::string, Handler);
    [[nodiscard]] std::string const& label() const;
    void run();

private:
    std::string m_label;
    Handler m_handler;
};

using MenuEntries = std::vector<MenuEntry>;

struct MenuDescription {
    std::string title;
    MenuEntries entries;
};

using MenuDescriptions = std::vector<MenuDescription>;

class Menu {
public:
    Menu(int, MenuDescription const&);
    [[nodiscard]] std::string const& title() const;
    [[nodiscard]] MenuEntries const& entries() const;
    [[nodiscard]] int selected() const;
    void render() const;
    void up();
    void down();
    void enter();

private:
    int m_column;
    std::string m_title;
    MenuEntries m_entries {};
    int m_selected { 0 };
};

class MenuBar : public WindowedWidget {
public:
    explicit MenuBar(MenuDescriptions const&);
    [[nodiscard]] bool active() const;
    void left();
    void right();
    void up();
    void down();
    void enter();
    [[nodiscard]] bool handle(KeyCode) override;
    [[nodiscard]] size_t selected() const;
    void render() override;

private:
    Menus m_entries;

    int m_bar_width;
    bool m_active { false };
    size_t m_selected { 0 };
};

}
