#pragma once
#include"ssa.hpp"
#include<ranges>
#include<vector>
#include<list>
namespace bbe::targets::ljf::impl{
    class ProcedureIC{
        using insv_t = std::list<ssa::Instruction>;
        insv_t _instructions;
        std::vector<insv_t::iterator> _labels;
        public:
            ProcedureIC(const ssa::ProcedureIC& ssaf);
            const std::list<ssa::Instruction>& instructions() const{
                return _instructions;
            }
            // sorted
            const std::vector<insv_t::iterator>& labels() const{
                return _labels;
            }
    };
}
namespace bbe::targets::ljf{
    BBE_EXPORT ProcedureIC;
}

