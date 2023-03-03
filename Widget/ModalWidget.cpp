/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "App.h"
#include <Widget/Widget.h>

namespace Scratch {

ModalWidget::ModalWidget(int w, int h)
    : m_width(w)
    , m_height(h)
{
}

int ModalWidget::width() const
{
    return m_width;
}

int ModalWidget::height() const
{
    return m_height;
}

int ModalWidget::top() const
{
    return (App::instance().height() - height()) / 2;
}

int ModalWidget::left() const
{
    return (App::instance().width() - width()) / 2;
}

void ModalWidget::dismiss()
{
    if (App::instance().modal() == this)
        App::instance().dismiss_modal();
}

}
