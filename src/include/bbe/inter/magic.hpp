#pragma once
#include"value.hpp"
namespace bbe::inter::impl{
    Value eval_intrin(std::uint32_t intrin,const std::vector<Value>& argv);
}
namespace bbe::inter{
    BBE_EXPORT eval_intrin;
}
