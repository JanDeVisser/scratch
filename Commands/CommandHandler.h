/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Commands/Command.h>
#include <Widget/Widget.h>

namespace Scratch {

class CommandHandler : public ModalWidget {
public:
    explicit CommandHandler(ScheduledCommand&);
    void render() override;
    void argument_done(std::string);
    void abort();

private:
    Widget& m_owner;
    Command m_command;
    ModalWidget* m_current_handler { nullptr };
    std::vector<std::string> m_arguments;
    int m_current_parameter { 0 };
};

}
