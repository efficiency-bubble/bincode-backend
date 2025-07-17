#include<bbe/targets/ljf.hpp>
#include<bbe/inter/ljf.hpp>
#include<ranges>
#include<format>
#include<print>
namespace bbe::targets::ljf::impl{
    using BlockTransfer = std::vector<std::pair<std::uint32_t,std::uint32_t>>;
    void populate_tt(BlockTransfer& tt,const ssa::BasicBlock& src,const ssa::BasicBlock& dst){
        for(const auto& [n,v] : dst.imports()){
            tt.emplace_back(src.nametable().at(n),v);
        }
    }
    void write_transfers(std::vector<ssa::Instruction>& ins,const BlockTransfer& tt){
        for(const auto& [from,to] : tt){
            ins.emplace_back(ssa::Operation::MOV,to,std::vector<std::uint32_t>{from});
        }
    }
    ProcedureIC::ProcedureIC(const ssa::ProcedureIC& ssaf){
        std::vector<std::pair<std::uint32_t,BlockTransfer>> branched_transfer_table;
        std::vector<BlockTransfer> transfer_table;
        std::vector<std::vector<std::uint32_t>> transfer_id(ssaf.blocks().size());
        for(const auto& [out_ids,block] : std::views::zip(transfer_id,ssaf.blocks())){
            if(block.retcond()==block.NCOND){
                if(!block.retlocs().empty()){
                    out_ids.emplace_back(transfer_table.size());
                    populate_tt(transfer_table.emplace_back(),block,ssaf.blocks()[block.retlocs().front()]);
                }
            }else{
                for(const std::uint32_t dst : block.retlocs()){
                    out_ids.emplace_back(branched_transfer_table.size()+ssaf.blocks().size());
                    populate_tt(branched_transfer_table.emplace_back(std::piecewise_construct,std::forward_as_tuple(dst),std::tuple<>()).second,block,ssaf.blocks()[dst]);
                }
            }
        }
        for(const auto& [out_ids,block] : std::views::zip(transfer_id,ssaf.blocks())){
            _labels.emplace_back(_instructions.size());
            _instructions.append_range(block.instructions());
            if(block.retcond() == block.NCOND){
                if(!out_ids.empty()){
                    [[assume(block.retlocs().size()==1uz)]];
                    write_transfers(_instructions,transfer_table[out_ids.front()]);
                    _instructions.emplace_back(ssa::Operation::JMP,0,block.retlocs());
                }
            }else{
                _instructions.emplace_back(ssa::Operation::JMP,block.retcond(),out_ids);
            }
        }
        for(const auto& [next,trans] : branched_transfer_table){
            _labels.emplace_back(_instructions.size());
            write_transfers(_instructions,trans);
            _instructions.emplace_back(ssa::Operation::JMP,0,std::vector<std::uint32_t>{next});
        }
    }
}
namespace bbe::inter::ljf::impl{
    Value run(GlobalEnvironment&,const targets::ljf::ProcedureIC& fn,const std::vector<Value>& argv){
        FunctionLocals lc;
        std::uint32_t pc = 0;
        while(true){
            // std::print("{}: ",pc);
            // for(const auto& [k,v] : lc){
            //     std::print("{} = {} ; ",k,v.value().tell());
            // }
            // std::println();
            if(pc >= fn.instructions().size()){
                throw std::logic_error("inter::ljf::run(): Execution reached end of code without return");
            }
            const auto& ins = fn.instructions()[pc++];
            switch(ins.opcode){
                using enum targets::ssa::Operation;
                case PACK:
                    lc[ins.dst].value().emplace<pack>();
                    for(std::uint32_t sv : ins.src){
                        lc[ins.dst].get<pack>().values.emplace_back(lc.at(sv));
                    }
                    break;
                case CALL:
                    // TODO
                    throw std::logic_error("inter::ljf::run(): Function calling not yet supported");
                    break;
                case CMAG: {
                    const auto& packv = lc[ins.src.back()].get<pack>().values;
                    switch(ins.src.front()){
                        case 10: // add
                            lc[ins.dst].value().emplace<uint32v>(uint32v(packv.front().get<uint32v>().value+packv.back().get<uint32v>().value));
                            break;
                        default:
                            throw std::logic_error(std::format("inter::ljf::run(): Unknown magic function {}"sv,ins.src.front()));
                    }
                    break;
                }
                case RET:
                    return lc[ins.dst];
                case MOV:
                    lc[ins.dst] = lc[ins.src.front()];
                    break;
                case LDAR:
                    lc[ins.dst] = argv[ins.src.front()];
                    break;
                case JMP: {
                    if(ins.src.size()==1uz){
                        pc = fn.labels()[ins.src.front()];
                    }else{
                        const Value& cond = lc[ins.dst];
                        switch(cond.index()){
                            case Value::index_of<boolv>:
                                pc = fn.labels()[ins.src[cond.get<boolv>().value]];
                                break;
                            case Value::index_of<uint32v>:
                                pc = fn.labels()[ins.src[cond.get<uint32v>().value]];
                                break;
                            default:
                                throw std::logic_error(std::format("inter::ljf::run(): value of type {} cannot be used as a branch condition"sv,ins.src.front()));
                        }
                    }
                    break;
                }
                default:
                    throw std::logic_error(cppp::tocs(u8"inter::ljf::run(): Illegal instruction "s+stringify_enum(ins.opcode)));
            }
        }
    }
}
