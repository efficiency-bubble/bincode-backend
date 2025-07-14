#include<bbe/targets/ljf.hpp>
#include<bbe/inter/ljf.hpp>
namespace bbe::targets::ljf::impl{
    ProcedureIC::ProcedureIC(const ssa::ProcedureIC& ssaf){
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
                            throw std::logic_error("inter::ljf::run(): Unknown magic function "s+std::to_string(lc[ins.dst].get<magic_ref>().value));
                    }
                    break;
                }
                case RET:
                    return lc[ins.dst];
                case LDAR:
                    lc[ins.dst] = argv[ins.src.front()];
                    break;
                default:
                    throw std::logic_error(cppp::tocs(u8"inter::ljf::run(): Illegal instruction "s+stringify_enum(ins.opcode)));
            }
        }
    }
}
