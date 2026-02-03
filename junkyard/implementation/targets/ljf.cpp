#include<bbe/targets/ljf.hpp>
#include<unordered_map>
#include<ranges>
namespace bbe::targets::ljf::impl{
    using insv_t = std::list<ssa::Instruction>;
    using labv_t = std::vector<insv_t::iterator>;
    void write_transfers(insv_t& insv,labv_t& labels,const ssa::BasicBlock& from,const ssa::BasicBlock& to,std::uint32_t lb){
        auto it = to.imports().begin();
        if(it==to.imports().end()){
            labels.emplace_back(insv.emplace(insv.end(),ssa::Operation::JMP,lb));
        }else{
            labels.emplace_back(insv.emplace(insv.end(),ssa::Operation::MOV,it->second,std::vector<std::uint32_t>{from.nametable().at(it->first)}));
            while(++it!=to.imports().end()){
                insv.emplace_back(ssa::Operation::MOV,it->second,std::vector<std::uint32_t>{from.nametable().at(it->first)});
            }
            insv.emplace_back(ssa::Operation::JMP,lb);
        }
    }
    ProcedureIC::ProcedureIC(const ssa::ProcedureIC& ssaf){
        std::vector<std::uint32_t> block_labels;
        {
            std::uint32_t lc = 0;
            for(const auto& block : ssaf.blocks()){
                block_labels.emplace_back(lc++);
                if(block.retcond()==block.NCOND){
                    if(block.ret()==block.NRET) continue;
                    ++lc;
                }else{
                    ++++lc;
                }
            }
        }
        for(const auto& [i,block] : std::views::enumerate(ssaf.blocks())){
            std::unordered_map<std::uint32_t,std::uint32_t> val_map;
            _labels.emplace_back(_instructions.insert_range(_instructions.end(),block.instructions()));
            if(block.retcond()==block.NCOND){
                if(std::uint32_t rd=block.ret();rd!=block.NRET){
                    write_transfers(_instructions,_labels,block,ssaf.blocks()[rd],block_labels[rd]);
                }
            }else{
                _instructions.emplace_back(ssa::Operation::JCC,_labels.size(),std::vector<std::uint32_t>{block.retcond()});
                write_transfers(_instructions,_labels,block,ssaf.blocks()[block.ret2()],block_labels[block.ret2()]);
                write_transfers(_instructions,_labels,block,ssaf.blocks()[block.ret()],block_labels[block.ret()]);
            }
        }
    }
}
