/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <ncurses.h>

#include <App.h>
#include <Widget.h>

namespace Scratch {

class MessageBar : public WindowedWidget, public Messenger {
public:
    MessageBar();
    void vmessage(char const*, va_list) override;
    void clear();
    void render() override;
private:
    std::string m_message;
};

}
