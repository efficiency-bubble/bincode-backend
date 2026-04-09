#pragma once
#include"commons.hpp"
#include"../commons.hpp"
#include"dfg.hpp"
#include<cppp/string.hpp> // enum debug
#include<limits>
#include<vector>
#include<list>
namespace bbe::targets::rtl::impl{
    using namespace std::literals;
    BBE_DEBUG_NAMED_ENUM(Operation,CALL,LDFN,LDI,ARG,IPACK,MKPACK,PACKATT,ADD,SUB,RET,JMP,JF,MOV,CLE,CEQ,PRI); // TODO: Remove PRI
    struct Instruction{
        Operation opcode;
        std::uint32_t dst;
        std::uint32_t src;
    };
    class Function{
        friend class FunctionCompiler;
        using insv_t = std::list<Instruction>;
        public:
            using ip_t = insv_t::const_iterator;
        private:
            insv_t ins;
            // stores one before the target statement
            std::vector<ip_t> labels;
            std::uint32_t nvals;
        public:
            Function(const dfg::DataFlowGraph&);
            const std::list<Instruction>& instructions() const{
                return ins;
            }
            // returns one before the target statement
            ip_t get_label(std::size_t i) const{
                return labels[i];
            }
            std::uint32_t num_vals() const{
                return nvals;
            }
    };
}
namespace bbe::targets::rtl{
    BBE_EXPORT Operation;
    BBE_EXPORT Instruction;
    BBE_EXPORT Function;
}
