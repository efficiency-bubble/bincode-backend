#include<bbe/targets/ljf.hpp>
#include<bbe/inter/ljf.hpp>
#include<ranges>
#include<format>
namespace bbe::targets::ljf::impl{
    struct BlockTransfer{
        std::vector<std::pair<std::uint32_t,std::uint32_t>> moves;
        std::uint32_t to;
        BlockTransfer(std::uint32_t to) : moves(), to(to){}
    };
    ProcedureIC::ProcedureIC(const ssa::ProcedureIC& ssaf){
        std::vector<BlockTransfer> transfer_table;
        std::vector<std::vector<std::uint32_t>> transfer_id(ssaf.blocks().size());
        for(const auto& [out_ids,block] : std::views::zip(transfer_id,ssaf.blocks())){
            for(const std::uint32_t dst : block.retlocs()){
                out_ids.emplace_back(transfer_table.size()+ssaf.blocks().size());
                BlockTransfer& entry = transfer_table.emplace_back(dst);
                for(const auto& [n,v] : ssaf.blocks()[dst].imports()){
                    entry.moves.emplace_back(block.nametable().at(n),v);
                }
            }
        }
        for(const auto& [out_ids,block] : std::views::zip(transfer_id,ssaf.blocks())){
            _labels.emplace_back(_instructions.size());
            _instructions.append_range(block.instructions());
            if(block.retcond() == block.NCOND){
                if(!out_ids.empty()){
                    [[assume(out_ids.size()==1uz)]];
                    _instructions.emplace_back(ssa::Operation::JMP,0,out_ids);
                }
            }else{
                _instructions.emplace_back(ssa::Operation::JMP,block.retcond(),out_ids);
            }
        }
        for(const BlockTransfer& trans : transfer_table){
            _labels.emplace_back(_instructions.size());
            for(const auto& [from,to] : trans.moves){
                _instructions.emplace_back(ssa::Operation::MOV,to,std::vector<std::uint32_t>{from});
            }
            _instructions.emplace_back(ssa::Operation::JMP,0,std::vector<std::uint32_t>{trans.to});
        }
    }
}
namespace bbe::inter::ljf::impl{
    Value run(GlobalEnvironment&,const targets::ljf::ProcedureIC& fn,const std::vector<Value>& argv){
        FunctionLocals lc;
        std::uint32_t pc = 0;
        while(true){
            // for(const auto& [k,v] : lc){
            //     std::cout << k << '=' << v.value().tell() << " ; ";
            // }
            // std::cout << '\n';
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
                                pc = fn.labels()[cond.get<boolv>().value];
                                break;
                            case Value::index_of<uint32v>:
                                pc = fn.labels()[cond.get<uint32v>().value];
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
