#pragma once
#include"../targets/dfg.hpp"
#include"value.hpp"
namespace bbe::inter::dfg::impl{
    using namespace bbe::inter::impl;
    struct FunctionCall{
        std::vector<Value> argv;
    };
    Value eval(FunctionCall&,const targets::dfg::DataNode* nd);
}
namespace bbe::inter::dfg{
    BBE_EXPORT FunctionCall;
    BBE_EXPORT eval;
}
