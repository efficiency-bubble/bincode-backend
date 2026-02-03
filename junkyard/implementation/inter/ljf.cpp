#include<bbe/inter/magic.hpp>
#include<bbe/inter/ljf.hpp>
#include<format>
#include<print>
namespace bbe::inter::ljf::impl{
    using namespace bbe::inter::impl;
    Value run(GlobalEnvironment&,const targets::ljf::ProcedureIC& fn,const std::vector<Value>& argv){
        FunctionLocals lc;
        std::list<targets::ssa::Instruction>::const_iterator pc = fn.instructions().begin();
        while(true){
            // std::print("{}: ",pc);
            // for(const auto& [k,v] : lc){
            //     std::print("{} = {} ; ",k,v.value().tell());
            // }
            // std::println();
            if(pc == fn.instructions().end()){
                throw std::logic_error("inter::ljf::run(): Execution reached end of code without return");
            }
            const auto& ins = *(pc++);
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
                case CMAG:
                    lc[ins.dst] = cmag(ins.src.front(),lc[ins.src[1uz]]);
                    break;
                case RET:
                    return lc[ins.dst];
                case MOV:
                    lc[ins.dst] = lc[ins.src.front()];
                    break;
                case LDAR:
                    lc[ins.dst] = argv[ins.src.front()];
                    break;
                case JMP:
                    pc = fn.labels()[ins.dst];
                    break;
                case JCC: {
                    const Value& cond = lc[ins.src.front()];
                    switch(cond.tell()){
                        case Value::index_of<boolv>:
                            if(cond.get<boolv>().value){
                                pc = fn.labels()[ins.dst];
                            }
                            break;
                        case Value::index_of<uint32v>:
                            if(cond.get<uint32v>().value){
                                pc = fn.labels()[ins.dst];
                            }
                            break;
                        default:
                            throw std::logic_error(std::format("inter::ljf::run(): value of type {} cannot be used as a branch condition"sv,cond.tell()));
                    }
                    break;
                }
                default:
                    throw std::logic_error(cppp::tocs(u8"inter::ljf::run(): Illegal instruction "s+stringify_enum(ins.opcode)));
            }
        }
    }
}
