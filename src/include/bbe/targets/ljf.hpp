#pragma once
#include"ssa.hpp"
#include<ranges>
#include<variant>
namespace bbe::targets::ljf::impl{
    class ProcedureIC{
        std::vector<ssa::Instruction> _instructions;
        std::vector<std::uint32_t> _labels;
        public:
            ProcedureIC(const ssa::ProcedureIC& ssaf);
            const std::vector<ssa::Instruction>& instructions() const{
                return _instructions;
            }
            // sorted
            const std::vector<std::uint32_t>& labels() const{
                return _labels;
            }
    };
}
namespace bbe::targets::ljf{
    BBE_EXPORT ProcedureIC;
}

