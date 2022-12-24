/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <App.h>
#include <StatusBar.h>
#include <Widget.h>

namespace Scratch {

class LogViewer : public WindowedWidget, public Logger, public StatusReporter {
public:
    LogViewer();

    void vlog(char const*, va_list) override;
    void render() override;
    [[nodiscard]] bool handle(int) override;
    std::string status() override { return "Log Viewer"; }
private:
    strings m_messages;
};

}
