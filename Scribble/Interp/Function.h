/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Scribble/Interp/Value.h>

namespace Scratch::Interp {

class Function {
public:
    Function(std::string);
    virtual ~Function() = default;
    std::string const& name() const;
    [[nodiscard]] virtual Value execute(std::vector<Value> const&) const = 0;
    [[nodiscard]] virtual std::string to_string() const;
private:
    std::string m_name;
};

using pFunction = std::shared_ptr<Function>;

}
