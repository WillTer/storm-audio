#pragma once

#include <type_traits>

namespace type_traits
{

template <typename T, bool = std::is_enum_v<T>>
struct is_flag;

template <typename T>
struct is_flag<T, true>: std::false_type {
};

}  // namespace type_traits

// Bit operators for flag types
template <typename T>
    requires type_traits::is_flag<T>::value
constexpr T operator|(T l, T r)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(l) | static_cast<std::underlying_type_t<T>>(r));
}

template <typename T>
    requires type_traits::is_flag<T>::value
constexpr T operator&(T l, T r)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(l) & static_cast<std::underlying_type_t<T>>(r));
}

template <typename T>
    requires type_traits::is_flag<T>::value
constexpr T operator^(T l, T r)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(l) ^ static_cast<std::underlying_type_t<T>>(r));
}

template <typename T>
    requires type_traits::is_flag<T>::value
constexpr T operator~(T l)
{
    return static_cast<T>(~static_cast<std::underlying_type_t<T>>(l));
}

template <typename T>
    requires type_traits::is_flag<T>::value
constexpr bool is_flag_enabled(T value, T flag)
{
    return (value & flag) == flag;
}
