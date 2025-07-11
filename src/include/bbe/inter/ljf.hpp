#pragma once
#include"../targets/ljf.hpp"
#include"value.hpp"
#include<map>
#include<iostream>
namespace bbe::inter::ljf::impl{
    using namespace std::literals;
    using FunctionLocals = std::map<std::uint32_t,Value>;
    class GlobalEnvironment{};
    Value run(GlobalEnvironment&,const targets::ljf::ProcedureIC& fn,const std::vector<Value>& argv={}){
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
                    if(lc[ins.src.front()].holds<magic_ref>()){
                        const std::vector<Value>& packv = lc[ins.src.back()].get<pack>().values;
                        switch(lc[ins.src.front()].get<magic_ref>().value){
                            case 10: // add
                                lc[ins.dst].value().emplace<uint32v>(uint32v(packv.front().get<uint32v>().value+packv.back().get<uint32v>().value));
                                break;
                            default:
                                throw std::logic_error("inter::ljf::run(): Unknown magic function "s+std::to_string(lc[ins.dst].get<magic_ref>().value));
                        }
                    }else{
                        throw std::logic_error("inter::ljf::run(): Illegal call");
                    }
                    break;
                case MAGIC:
                    lc[ins.dst].value().emplace<magic_ref>(ins.src.front());
                    break;
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
namespace bbe::inter::ljf{
    BBE_EXPORT GlobalEnvironment;
    BBE_EXPORT run;
}
