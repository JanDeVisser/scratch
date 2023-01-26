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


template <typename T, typename A1, typename ...Args>
constexpr bool CheckParameterType()
{
    return (std::is_convertible<A1,T>());
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
    requires (CheckParameterPackSize<Dim, Args...>() && CheckParameterType<T, Args...>())
        : coordinates { { std::forward<Args>(values)... } }
    {
    }

    const T& operator[](size_t idx) const
    {
        assert(idx < Dim);
        return coordinates[idx];
    }

    T& operator[](size_t idx)
    {
        assert(idx < Dim);
        return coordinates[idx];
    }

private:
};

struct Tuple {
    int coordinates[2] = { 0, 0 };
    Tuple(int x, int y)
    {
        coordinates[0] = x;
        coordinates[1] = y;
    }
    Tuple() = default;

    const int& operator[](size_t idx) const
    {
        assert(idx < 2);
        return coordinates[idx];
    }

    int& operator[](size_t idx)
    {
        assert(idx < 2);
        return coordinates[idx];
    }

    [[nodiscard]] std::string to_string() const
    {
        return Obelix::format("{}x{}", coordinates[0], coordinates[1]);
    }
};

struct Position : public Tuple {
    Position(int t, int l)
        : Tuple(t, l)
    {
    }
    Position() = default;

    [[nodiscard]] int left() const { return coordinates[0]; }
    [[nodiscard]] int top() const { return coordinates[1]; }
};

struct Size : public Tuple {
    Size(int w, int h)
        : Tuple(w, h)
    {
    }
    Size() = default;

    [[nodiscard]] int width() const { return coordinates[0]; }
    [[nodiscard]] int height() const { return coordinates[1]; }
    [[nodiscard]] bool empty() const { return (coordinates[0] + coordinates[1]) == 0; }
};

struct Box {
    Box(int t, int l, int w, int h)
        : position(t, l)
        , size(w, h)
    {
    }
    Box(Position p, Size s)
        : position(std::move(p))
        , size(std::move(s))
    {
    }
    Box() = default;

    Position position;
    Size size;

    [[nodiscard]] int top() const { return position.top(); }
    [[nodiscard]] int left() const { return position.left(); }
    [[nodiscard]] int width() const { return size.width(); }
    [[nodiscard]] int height() const { return size.height(); }
    [[nodiscard]] bool empty() const { return size.empty(); }

    [[nodiscard]] std::string to_string() const
    {
        return Obelix::format("{}+{}", position, size);
    }
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
