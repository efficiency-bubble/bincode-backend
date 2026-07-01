#pragma once
#include"commons.hpp"
namespace bbe::impl{
    struct uninitialize_t{} constexpr inline uninitialize;
}
namespace bbe{
    BBE_EXPORT uninitialize_t;
    BBE_EXPORT uninitialize;
}
