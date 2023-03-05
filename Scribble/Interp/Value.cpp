/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Checked.h>

#include <Scribble/Interp/Operator.h>
#include <Scribble/Interp/Value.h>
#include <Scribble/Interp/Function.h>

namespace Scratch::Interp {

using namespace Obelix;

enum class ValueTypeWithCount {
#undef __ENUMERATE_TYPE
#define __ENUMERATE_TYPE(name, type) type,
    ENUMERATE_TYPES(__ENUMERATE_TYPE)
#undef __ENUMERATE_TYPE
        Count,
};

enum class TypeData : uint8_t {
    Null = 1 << 4,
    Int8 = 2 << 4,
    Int16 = 3 << 4,
    Int32 = 4 << 4,
    Int64 = 5 << 4,
    Uint8 = 6 << 4,
    Uint16 = 7 << 4,
    Uint32 = 8 << 4,
    Uint64 = 9 << 4,
};

template <std::integral Int>
constexpr bool is_within_range(auto value)
{
    return (value >= std::numeric_limits<Int>::min() && value <= std::numeric_limits<Int>::max());
}

template<typename Callback>
static decltype(auto) downsize_integer(Integer auto value, Callback&& callback)
{
    if constexpr (std::is_signed_v<decltype(value)>) {
        if (is_within_range<int8_t>(value))
            return callback(static_cast<int8_t>(value), TypeData::Int8);
        if (is_within_range<int16_t>(value))
            return callback(static_cast<int16_t>(value), TypeData::Int16);
        if (is_within_range<int32_t>(value))
            return callback(static_cast<int32_t>(value), TypeData::Int32);
        return callback(value, TypeData::Int64);
    } else {
        if (is_within_range<uint8_t>(value))
            return callback(static_cast<int8_t>(value), TypeData::Uint8);
        if (is_within_range<uint16_t>(value))
            return callback(static_cast<int16_t>(value), TypeData::Uint16);
        if (is_within_range<uint32_t>(value))
            return callback(static_cast<int32_t>(value), TypeData::Uint32);
        return callback(value, TypeData::Uint64);
    }
}

template<typename Callback>
static decltype(auto) downsize_integer(Value const& value, Callback&& callback)
{
    assert(value.is_int());
    if (std::holds_alternative<int64_t>(value.value()))
        return downsize_integer(std::get<int64_t>(value.value()), std::forward<Callback>(callback));
    return downsize_integer(std::get<uint64_t>(value.value()), std::forward<Callback>(callback));
}

template<typename Callback>
static Value perform_integer_operation(Value const& lhs, Value const& rhs, Callback&& callback)
{
    assert(lhs.is_int());
    assert(rhs.is_int());

    if (std::holds_alternative<int64_t>(lhs.value())) {
        if (auto rhs_value = rhs.to_int<int64_t>(); rhs_value.has_value())
            return callback(lhs.to_int<int64_t>().value(), rhs_value.value());
    } else {
        if (auto rhs_value = rhs.to_int<uint64_t>(); rhs_value.has_value())
            return callback(lhs.to_int<uint64_t>().value(), rhs_value.value());
    }

    return Value { ErrorCode::IntegerOverflow };
}

Value::Value(ValueType type)
    : m_type(type)
{
}

Value::Value(std::string value)
    : m_type(ValueType::Text)
    , m_value(std::move(value))
{
}

Value::Value(std::string_view value)
    : m_type(ValueType::Text)
    , m_value(std::string(value))
{
}

Value::Value(double value)
{
    if (trunc(value) == value) {
        if (is_within_range<int64_t>(value)) {
            m_type = ValueType::Integer;
            m_value = static_cast<int64_t>(value);
            return;
        }
        if (is_within_range<uint64_t>(value)) {
            m_type = ValueType::Integer;
            m_value = static_cast<uint64_t>(value);
            return;
        }
    }
    m_type = ValueType::Float;
    m_value = value;
}

Value::Value(ErrorCode value)
    : m_type(ValueType::Error)
    , m_value(value)
{
}

Value::Value(pFunction value)
    : m_type(ValueType::Function)
    , m_value(value)
{
}

Value::Value(Value&& other) noexcept
    : m_type(other.m_type)
    , m_value(std::move(other.m_value))
{
}

Value::~Value() = default;

ValueType Value::type() const
{
    return m_type;
}

std::string_view Value::type_name() const
{
    return ValueType_name(type());
}

bool Value::is_type_compatible_with(ValueType other_type) const
{
    switch (type()) {
    case ValueType::Null:
        return false;
    case ValueType::Integer:
    case ValueType::Float:
        return other_type == ValueType::Integer || other_type == ValueType::Float;
    default:
        break;
    }

    return type() == other_type;
}

bool Value::is_null() const
{
    return !m_value.has_value();
}

bool Value::is_int() const
{
    return m_value.has_value() && (std::holds_alternative<int64_t>(m_value.value()) || std::holds_alternative<uint64_t>(m_value.value()));
}

bool Value::is_string() const
{
    return m_value.has_value() && std::holds_alternative<std::string>(m_value.value());
}

bool Value::is_error() const
{
    return m_value.has_value() && std::holds_alternative<ErrorCode>(m_value.value());
}

bool Value::is_function() const
{
    return m_value.has_value() && std::holds_alternative<pFunction>(m_value.value());
}

std::string Value::to_string() const
{
    if (is_null())
        return "(null)";

    return visit(
        [](std::string const& value) -> std::string { return value; },
        [](Integer auto value) -> std::string { return Obelix::to_string<decltype(value)>()(value); },
        [](double value) -> std::string { return Obelix::to_string<double>()(value); },
        [](bool value) -> std::string { return value ? "true" : "false"; },
        [](ErrorCode value) -> std::string { return ErrorCode_name(value); },
        [](pFunction const& value) -> std::string { return value->to_string(); });
}

std::optional<double> Value::to_double() const
{
    if (is_null())
        return {};

    return visit(
        [](std::string const& value) -> std::optional<double> { return Obelix::try_to_double<std::string>()(value); },
        [](Integer auto value) -> std::optional<double> { return static_cast<double>(value); },
        [](double value) -> std::optional<double> { return value; },
        [](bool value) -> std::optional<double> { return static_cast<double>(value); },
        [](ErrorCode value) -> std::optional<double> { return static_cast<double>(value); },
        [](pFunction const& value) -> std::optional<double> { return {}; });
}

std::optional<bool> Value::to_bool() const
{
    if (is_null())
        return {};

    return visit(
        [](std::string const& value) -> std::optional<bool> {
            if (stricmp(value.c_str(), "true") == 0 || stricmp(value.c_str(), "t") == 0)
                return true;
            if (stricmp(value.c_str(), "false") == 0 || stricmp(value.c_str(), "f") == 0)
                return false;
            return {};
        },
        [](Integer auto value) -> std::optional<bool> { return static_cast<bool>(value); },
        [](double value) -> std::optional<bool> { return fabs(value) > std::numeric_limits<double>::epsilon(); },
        [](bool value) -> std::optional<bool> { return value; },
        [](ErrorCode value) -> std::optional<bool> { return value == ErrorCode::NoError; },
        [](pFunction const& value) -> std::optional<bool> { return {}; });
}

std::optional<ErrorCode> Value::to_error() const
{
    if (is_null())
        return {};

    return visit(
        [](std::string const& value) -> std::optional<ErrorCode> { return {}; },
        [](Integer auto value) -> std::optional<ErrorCode> { return {}; },
        [](double value) -> std::optional<ErrorCode> { return {}; },
        [](bool value) -> std::optional<ErrorCode> { return {}; },
        [](ErrorCode value) -> std::optional<ErrorCode> { return value; },
        [](pFunction const& value) -> std::optional<ErrorCode> { return {}; });
}

std::optional<pFunction> Value::to_function() const
{
    if (is_null())
        return {};

    return visit(
        [](std::string const& value) -> std::optional<pFunction> { return {}; },
        [](Integer auto value) -> std::optional<pFunction> { return {}; },
        [](double value) -> std::optional<pFunction> { return {}; },
        [](bool value) -> std::optional<pFunction> { return {}; },
        [](ErrorCode value) -> std::optional<pFunction> { return {}; },
        [](pFunction const& value) -> std::optional<pFunction> { return value; });
}

Value& Value::operator=(Value value)
{
    m_type = value.m_type;
    m_value = std::move(value.m_value);
    return *this;
}

Value& Value::operator=(std::string value)
{
    m_type = ValueType::Text;
    m_value = std::move(value);
    return *this;
}

Value& Value::operator=(double value)
{
    m_type = ValueType::Float;
    m_value = value;
    return *this;
}

Value& Value::operator=(ErrorCode value)
{
    m_type = ValueType::Error;
    m_value = value;
    return *this;
}

Value& Value::operator=(pFunction const& value)
{
    m_type = ValueType::Function;
    m_value = value;
    return *this;
}

int Value::compare(Value const& other) const
{
    if (is_null())
        return -1;
    if (other.is_null())
        return 1;

    return visit(
        [&](std::string const& value) -> int { return value.compare(other.to_string()); },
        [&](Integer auto value) -> int {
            auto casted = other.to_int<IntegerType<decltype(value)>>();
            if (!casted.has_value())
                return 1;

            if (value == *casted)
                return 0;
            return value < *casted ? -1 : 1;
        },
        [&](double value) -> int {
            auto casted = other.to_double();
            if (!casted.has_value())
                return 1;

            auto diff = value - *casted;
            if (fabs(diff) < std::numeric_limits<double>::epsilon())
                return 0;
            return diff < 0 ? -1 : 1;
        },
        [&](bool value) -> int {
            auto casted = other.to_bool();
            if (!casted.has_value())
                return 1;
            return value ^ *casted;
        },
        [&](ErrorCode value) -> int {
            auto casted = other.to_error();
            if (!casted.has_value())
                return 1;

            if (value == *casted)
                return 0;
            return value < *casted ? -1 : 1;
        },
        [&](pFunction const& value) -> int {
            auto casted = other.to_function();
            if (!casted.has_value())
                return 1;

            if (value == *casted)
                return 0;
            return 1;
        });
}

bool Value::operator==(Value const& value) const
{
    return compare(value) == 0;
}

bool Value::operator==(std::string_view const& value) const
{
    return to_string() == value;
}

bool Value::operator==(double value) const
{
    return to_double() == value;
}

bool Value::operator==(pFunction const& value) const
{
    return to_function() == value;
}

bool Value::operator!=(Value const& value) const
{
    return compare(value) != 0;
}

bool Value::operator<(Value const& value) const
{
    return compare(value) < 0;
}

bool Value::operator<=(Value const& value) const
{
    return compare(value) <= 0;
}

bool Value::operator>(Value const& value) const
{
    return compare(value) > 0;
}

bool Value::operator>=(Value const& value) const
{
    return compare(value) >= 0;
}

Value Value::add(Value const& other) const
{
    if (is_int() && other.is_int()) {
        return perform_integer_operation(*this, other, [](auto lhs, auto rhs) -> Value {
            Checked result { lhs };
            result.add(rhs);

            if (result.has_overflow())
                return Value { ErrorCode::IntegerOverflow };
            return Value { result.value_unchecked() };
        });
    }

    if (is_string() && other.is_string())
        return Value { to_string() + other.to_string() };

    auto lhs = to_double();
    auto rhs = other.to_double();

    if (!lhs.has_value() || !rhs.has_value())
        return Value { ErrorCode::ArgumentTypeMismatch };
    return Value { lhs.value() + rhs.value() };
}

Value Value::subtract(Value const& other) const
{
    if (is_int() && other.is_int()) {
        return perform_integer_operation(*this, other, [](auto lhs, auto rhs) -> Value {
            Checked result { lhs };
            result.sub(rhs);

            if (result.has_overflow())
                return Value { ErrorCode::IntegerOverflow };
            return Value { result.value_unchecked() };
        });
    }

    auto lhs = to_double();
    auto rhs = other.to_double();

    if (!lhs.has_value() || !rhs.has_value())
        return Value { ErrorCode::ArgumentTypeMismatch };
    return Value { lhs.value() - rhs.value() };
}

Value Value::multiply(Value const& other) const
{
    if (is_int() && other.is_int()) {
        return perform_integer_operation(*this, other, [](auto lhs, auto rhs) -> Value {
            Checked result { lhs };
            result.mul(rhs);

            if (result.has_overflow())
                return Value { ErrorCode::IntegerOverflow };
            return Value { result.value_unchecked() };
        });
    }

    auto lhs = to_double();
    auto rhs = other.to_double();

    if (!lhs.has_value() || !rhs.has_value())
        return Value { ErrorCode::ArgumentTypeMismatch };
    return Value { lhs.value() * rhs.value() };
}

Value Value::divide(Value const& other) const
{
    auto lhs = to_double();
    auto rhs = other.to_double();

    if (!lhs.has_value() || !rhs.has_value())
        return Value { ErrorCode::ArgumentTypeMismatch };
    if (rhs == 0.0)
        return Value { ErrorCode::IntegerOverflow };

    return Value { lhs.value() / rhs.value() };
}

Value Value::modulo(Value const& other) const
{
    if (!is_int() || !other.is_int())
        return Value { ErrorCode::ArgumentTypeMismatch };

    return perform_integer_operation(*this, other, [](auto lhs, auto rhs) -> Value {
        Checked result { lhs };
        result.mod(rhs);

        if (result.has_overflow())
            return Value { ErrorCode::IntegerOverflow };
        return Value { result.value_unchecked() };
    });
}

Value Value::negate() const
{
    if (type() == ValueType::Integer) {
        auto value = to_int<int64_t>();
        if (!value.has_value())
            return Value { ErrorCode::ArgumentTypeMismatch };

        return Value { value.value() * -1 };
    }

    if (type() == ValueType::Float)
        return Value { -to_double().value() };

    return Value { ErrorCode::ArgumentTypeMismatch };
}

Value Value::shift_left(Value const& other) const
{
    if (!is_int() || !other.is_int())
        return Value { ErrorCode::ArgumentTypeMismatch };

    return perform_integer_operation(*this, other, [](auto lhs, auto rhs) -> Value {
        using LHS = decltype(lhs);
        using RHS = decltype(rhs);

        static constexpr auto max_shift = static_cast<RHS>(sizeof(LHS) * 8);
        if (rhs < 0 || rhs >= max_shift)
            return Value { ErrorCode::IntegerOverflow };

        return Value { lhs << rhs };
    });
}

Value Value::shift_right(Value const& other) const
{
    if (!is_int() || !other.is_int())
        return Value { ErrorCode::ArgumentTypeMismatch };

    return perform_integer_operation(*this, other, [](auto lhs, auto rhs) -> Value {
        using LHS = decltype(lhs);
        using RHS = decltype(rhs);

        static constexpr auto max_shift = static_cast<RHS>(sizeof(LHS) * 8);
        if (rhs < 0 || rhs >= max_shift)
            return Value { ErrorCode::IntegerOverflow };

        return Value { lhs >> rhs };
    });
}

Value Value::bitwise_or(Value const& other) const
{
    if (!is_int() || !other.is_int())
        return Value { ErrorCode::ArgumentTypeMismatch };

    return perform_integer_operation(*this, other, [](auto lhs, auto rhs) {
        return Value { lhs | rhs };
    });
}

Value Value::bitwise_and(Value const& other) const
{
    if (!is_int() || !other.is_int())
        return Value { ErrorCode::ArgumentTypeMismatch };

    return perform_integer_operation(*this, other, [](auto lhs, auto rhs) {
        return Value { lhs & rhs };
    });
}

Value Value::bitwise_not() const
{
    if (!is_int())
        return Value { ErrorCode::ArgumentTypeMismatch };

    return downsize_integer(*this, [](auto value, auto) {
        return Value { ~value };
    });
}

}
