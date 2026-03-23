#pragma once
#include"value.hpp"
namespace bbe::inter::impl{
    Value cmag(std::uint32_t magic,const std::vector<Value>& arg);
}
namespace bbe::inter{
    BBE_EXPORT cmag;
}
