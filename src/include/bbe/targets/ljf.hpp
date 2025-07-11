#pragma once
#include"ssa.hpp"
#include<ranges>
#include<variant>
namespace bbe::targets::ljf::impl{
    class ProcedureIC{
        std::vector<ssa::Instruction> _instructions;
        std::vector<std::uint32_t> _labels;
        public:
            ProcedureIC(const ssa::ProcedureIC& ssaf){
                for(const auto& block : ssaf.blocks()){
                    _labels.emplace_back(_instructions.size());
                    _instructions.append_range(block.instructions());
                    if(block.retcond() == block.NCOND){
                        if(!block.retlocs().empty()){
                            [[assume(block.retlocs().size()==1uz)]];
                            _instructions.emplace_back(ssa::Operation::JMP,0,block.retlocs());
                        }
                    }else{
                        _instructions.emplace_back(ssa::Operation::JMP,block.retcond(),block.retlocs());
                    }
                }
            }
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

