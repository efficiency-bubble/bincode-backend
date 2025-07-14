#pragma once
#include"../targets/ljf.hpp"
#include"value.hpp"
#include<map>
#include<iostream>
namespace bbe::inter::ljf::impl{
    using namespace std::literals;
    using FunctionLocals = std::map<std::uint32_t,Value>;
    class GlobalEnvironment{};
    Value run(GlobalEnvironment&,const targets::ljf::ProcedureIC& fn,const std::vector<Value>& argv={});
}
namespace bbe::inter::ljf{
    BBE_EXPORT GlobalEnvironment;
    BBE_EXPORT run;
}
