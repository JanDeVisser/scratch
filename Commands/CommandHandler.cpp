/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/StringUtil.h>

#include <Commands/ArgumentHandler.h>
#include <Commands/CommandHandler.h>

using namespace Obelix;

namespace Scratch {

CommandHandler::CommandHandler(ScheduledCommand& scheduled_command)
    : ModalWidget(App::instance().width() / 2, App::instance().height() / 2)
    , m_owner(scheduled_command.owner)
    , m_command(scheduled_command.command)
{
}

void CommandHandler::render()
{
    if (m_current_parameter < 0) {
        dismiss();
        return;
    }
    if (m_current_parameter >= m_command.parameters.size()) {
        m_command.function(m_owner, m_arguments);
        dismiss();
        return;
    }
    if (m_current_handler == nullptr) {
        auto const& parameter = m_command.parameters[m_current_parameter];
        m_current_handler = create_argument_handler(this, parameter);
        App::instance().add_modal(m_current_handler);
    }
}

void CommandHandler::argument_done(std::string value)
{
    m_arguments.emplace_back(std::move(value));
    if (m_current_handler != nullptr)
        m_current_handler->dismiss();
    m_current_handler = nullptr;
    m_current_parameter++;
}

void CommandHandler::abort()
{
    if (m_current_handler != nullptr)
        m_current_handler->dismiss();
    m_current_handler = nullptr;
    m_current_parameter = -1;
}

}
