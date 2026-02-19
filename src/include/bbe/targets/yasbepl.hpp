#pragma once
#include"dfg.hpp"
#include<cppp/string.hpp> // output
namespace bbe::targets::yasbepl::impl{
    void compile(const dfg::DataFlowGraph& dfg,cppp::str& code);
}
namespace bbe::targets::yasbepl{
    BBE_EXPORT compile;
}
