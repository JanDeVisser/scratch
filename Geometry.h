/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <valarray>

#include <core/Logging.h>

namespace Scratch {

template <size_t Dim, typename ...Args>
constexpr bool CheckParameterPackSize()
{
    return sizeof...(Args) == Dim;
}

template <typename T, size_t Dim=2>
struct Vector {
    std::valarray<T> coordinates { Dim };

    Vector()
    {
        for (auto ix = 0u; ix < Dim; ++ix)
            coordinates[ix] = 0;
    }

    template <typename ...Args>
    explicit Vector(Args... values)
    requires (CheckParameterPackSize<Dim, Args...>())
        : coordinates { { values... } }
    {
    }

    const T& operator[](size_t idx) const
    {
        assert(idx < Dim);
        return coordinates[idx];
    }

    float& operator[](size_t idx)
    {
        assert(idx < Dim);
        return coordinates[idx];
    }

private:
};

struct Vec2 : public Vector<float,2> {
    Vec2(float x, float y)
        : Vector(x, y)
    {
    }
    Vec2() = default;

    [[nodiscard]] float x() const { return coordinates[0]; }
    [[nodiscard]] float y() const { return coordinates[1]; }
};

struct Rect : public Vector<float,4> {
    Rect(float x, float y, float w, float h)
        : Vector(x, y, w, h)
    {
    }

    [[nodiscard]] float x() const { return coordinates[0]; }
    [[nodiscard]] float y() const { return coordinates[1]; }
    [[nodiscard]] float width() const { return coordinates[2]; }
    [[nodiscard]] float height() const { return coordinates[3]; }
};

template<typename T>
T clamp(T v, T lo, T hi)
{
    assert(lo <= hi);
    if (v < lo)
        v = lo;
    if (v > hi)
        v = hi;

    return v;
}

template<typename T>
bool intersects(T px, T py, T x0, T y0, T x1, T y1)
{
    return (px > std::min(x0, x1) && px < std::max(x0, x1)) && (py > std::min(y0, y1) && py < std::max(y0, y1));
}

}
