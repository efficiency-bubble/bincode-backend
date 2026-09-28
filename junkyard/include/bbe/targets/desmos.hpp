#pragma once
#include"commons.hpp"
#include"../commons.hpp"
#include"dfg.hpp"
#include<cppp/string.hpp> // output
namespace bbe::targets::desmos::impl{
    using namespace bbe::impl;
    using namespace bbe::targets::impl;
    void compile(cppp::str& out,const dfg::DataFlowGraph& fn,cppp::sv prefix);
}
namespace bbe::targets::desmos{
    BBE_EXPORT compile;
}
