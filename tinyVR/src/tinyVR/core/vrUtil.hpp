#pragma once

#include <functional>

#include "tinyVR/core/vrMagicEnum.hpp"

namespace tinyvr
{
    template <typename EnumType>
    constexpr typename std::enable_if_t<std::is_enum_v<EnumType>, size_t>
    EnumElements()
    {
        return magic_enum::enum_count<EnumType>();
    }

    template <typename EnumType>
    constexpr typename std::enable_if_t<std::is_enum_v<EnumType>,
        std::underlying_type_t<EnumType>>
    EnumNumber(const EnumType& e)
    {
        return static_cast<typename std::underlying_type_t<EnumType>>(e);
    }
}