#pragma once
#include<cstdint>
namespace bbe::impl{
    using func_id = std::uint32_t;
    using type_id = std::uint32_t;
}
namespace bbe{
    BBE_EXPORT func_id;
    BBE_EXPORT type_id;
}
