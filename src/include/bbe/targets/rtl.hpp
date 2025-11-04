#pragma once
#include"commons.hpp"
#include"../commons.hpp"
#include"dfg.hpp"
#include<cppp/string.hpp> // enum debug
#include<limits>
#include<vector>
#include<list>
namespace bbe::targets::rtl::impl{
    using namespace bbe::impl;
    using namespace bbe::targets::impl;
    BBE_DEBUG_NAMED_ENUM(Operation,LDI,ADD,SUB,RET,JMP,JZ,MOV,PRI); // TODO: Remove PRI
    struct Instruction{
        Operation opcode;
        std::uint32_t dst;
        std::uint32_t src;
    };
    class Function{
        friend class FunctionCompiler;
        using insv_t = std::list<Instruction>;
        using ip_t = insv_t::const_iterator;
        insv_t ins;
        std::vector<std::size_t> labels;
        public:
            Function(const dfg::DataFlowGraph&);
            const std::list<Instruction>& instructions() const{
                return ins;
            }
    };
}
namespace bbe::targets::rtl{
    BBE_EXPORT Operation;
    BBE_EXPORT Instruction;
    BBE_EXPORT Function;
}
