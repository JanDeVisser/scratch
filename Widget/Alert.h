/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Widget/Widget.h>

namespace Scratch {

class Alert : public ModalWidget {
public:
    Alert(std::string);
    void render() override;
    bool dispatch(SDL_Keysym sym) override;

private:
    std::string m_text;
};

}
