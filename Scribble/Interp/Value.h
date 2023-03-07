/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cmath>
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <core/Error.h>
#include <core/Logging.h>
#include <core/StringUtil.h>

#include <Scribble/Syntax/Syntax.h>

namespace Scratch::Interp {

using namespace std::literals;
using namespace Obelix;

template<typename T>
concept Boolean = std::is_same_v<std::remove_cvref<T>, bool>;

template<typename T>
concept Integer = (std::is_integral_v<T> && !Boolean<T>);

#define ENUMERATE_TYPES(S)  \
    S("null", Null)         \
    S("text", Text)         \
    S("int", Integer)       \
    S("float", Float)       \
    S("bool", Boolean)      \
    S("function", Function) \
    S("error", Error)

enum class ValueType {
#undef __ENUMERATE_TYPE
#define __ENUMERATE_TYPE(name, type) type,
    ENUMERATE_TYPES(__ENUMERATE_TYPE)
#undef __ENUMERATE_TYPE
};

constexpr std::string_view ValueType_name(ValueType t)
{
    switch (t) {
#undef __ENUMERATE_TYPE
#define __ENUMERATE_TYPE(name, type) \
    case ValueType::type:                  \
        return name##sv;
        ENUMERATE_TYPES(__ENUMERATE_TYPE)
#undef __ENUMERATE_TYPE
    default:
        assert("Unreachable");
    }
}

class Function;
using pFunction = std::shared_ptr<Function>;

class Value {
    template<Integer T>
    using IntegerType = std::conditional_t<std::is_signed_v<T>, int64_t, uint64_t>;

public:
    explicit Value(ValueType type = ValueType::Null);
    explicit Value(std::string);
    explicit Value(std::string_view);
    explicit Value(double);
    explicit Value(ErrorCode);
    explicit Value(pFunction);
    Value(Value const&) noexcept = default;
    Value(Value&&) noexcept;
    ~Value();

    explicit Value(int64_t value)
        : m_type(ValueType::Integer)
        , m_value(value)
    {
    }

    explicit Value(uint64_t value)
        : m_type(ValueType::Integer)
        , m_value(value)
    {
    }

    explicit Value(Integer auto value)
        : m_type(ValueType::Integer)
        , m_value(static_cast<IntegerType<decltype(value)>>(value))
    {
    }

    explicit Value(Boolean auto value)
        : m_type(ValueType::Boolean)
        , m_value(value)
    {
    }

    [[nodiscard]] ValueType type() const;
    [[nodiscard]] std::string_view type_name() const;
    [[nodiscard]] bool is_type_compatible_with(ValueType) const;
    [[nodiscard]] bool is_null() const;
    [[nodiscard]] bool is_int() const;
    [[nodiscard]] bool is_string() const;
    [[nodiscard]] bool is_error() const;
    [[nodiscard]] bool is_function() const;

    [[nodiscard]] auto const& value() const
    {
        assert(m_value.has_value());
        return *m_value;
    }

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::optional<double> to_double() const;
    [[nodiscard]] std::optional<bool> to_bool() const;
    [[nodiscard]] std::optional<ErrorCode> to_error() const;
    [[nodiscard]] std::optional<pFunction> to_function() const;

    template<class... Fs>
    struct Visitor : Fs... {
        explicit Visitor(Fs&&... functions)
            : Fs(std::forward<Fs>(functions))...
        {
        }

        using Fs::operator()...;
    };

    template<typename... Fs>
    decltype(auto) visit(Fs&&... functions)
    {
        return std::visit(Visitor { std::forward<Fs>(functions)... }, m_value.value());
    }

    template<typename... Fs>
    decltype(auto) visit(Fs&&... functions) const
    {
        return std::visit(Visitor { std::forward<Fs>(functions)... }, m_value.value());
    }

    template<Integer T>
    [[nodiscard]] std::optional<T> to_int() const
    {
        if (is_null())
            return {};

        return visit(
            [](std::string const& value) -> std::optional<T> {
                if constexpr (std::is_signed_v<T>)
                    return Obelix::to_int<std::string, T>(value);
                else
                    return to_uint<std::string, T>(value);
            },
            [](Integer auto value) -> std::optional<T> {
                if (value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max())
                    return {};
                return static_cast<T>(value);
            },
            [](double value) -> std::optional<T> {
                if (value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max())
                    return {};
                return static_cast<T>(round(value));
            },
            [](bool value) -> std::optional<T> { return static_cast<T>(value); },
            [](ErrorCode value) -> std::optional<T> { return static_cast<T>(value); },
            [](pFunction const& value) -> std::optional<T> { return {}; });
    }

    Value& operator=(Value);
    Value& operator=(std::string);
    Value& operator=(double);
    Value& operator=(ErrorCode);
    Value& operator=(pFunction const&);

    Value& operator=(Integer auto value)
    {
        m_type = ValueType::Integer;
        m_value = static_cast<IntegerType<decltype(value)>>(value);
        return *this;
    }

    Value& operator=(Boolean auto value)
    {
        m_type = ValueType::Boolean;
        m_value = value;
        return *this;
    }

    [[nodiscard]] int compare(Value const&) const;
    bool operator==(Value const&) const;
    bool operator==(std::string_view const&) const;
    bool operator==(double) const;
    bool operator==(ErrorCode) const;
    bool operator==(pFunction const&) const;

    template<Integer T>
    bool operator==(T value)
    {
        return to_int<T>() == value;
    }

    bool operator!=(Value const&) const;
    bool operator<(Value const&) const;
    bool operator<=(Value const&) const;
    bool operator>(Value const&) const;
    bool operator>=(Value const&) const;

    [[nodiscard]] Value add(Value const&) const;
    [[nodiscard]] Value subtract(Value const&) const;
    [[nodiscard]] Value multiply(Value const&) const;
    [[nodiscard]] Value divide(Value const&) const;
    [[nodiscard]] Value modulo(Value const&) const;
    [[nodiscard]] Value negate() const;
    [[nodiscard]] Value shift_left(Value const&) const;
    [[nodiscard]] Value shift_right(Value const&) const;
    [[nodiscard]] Value bitwise_or(Value const&) const;
    [[nodiscard]] Value bitwise_and(Value const&) const;
    [[nodiscard]] Value bitwise_not() const;

private:
    using ValueImplType = std::variant<std::string, int64_t, uint64_t, double, bool, ErrorCode, pFunction>;

    ValueType m_type { ValueType::Null };
    std::optional<ValueImplType> m_value;
};

using Values = std::vector<Value>;

}

namespace Obelix {

using namespace Scratch::Interp;

template<>
struct to_string<Value const&> {
    std::string operator()(Value const& val)
    {
        return val.to_string();
    }
};

template<>
struct try_to_double<Value const&> {
    std::optional<double> operator()(Value const& val)
    {
        return val.to_double();
    }
};

template<>
struct try_to_long<Value const&> {
    std::optional<long> operator()(Value const& val)
    {
        return val.to_int<long>();
    }
};

}
