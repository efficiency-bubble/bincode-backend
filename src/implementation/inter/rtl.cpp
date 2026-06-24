#include<bbe/inter/rtl.hpp>
#include<cppp/array.hpp>
#include<iostream>
#include<stdexcept>
namespace bbe::inter::rtl::impl{
    using namespace cppp::literals;
    Value CompiledFunctionPool::call(func_id i,const Value& arg) const{
        const auto& fn = function(i);
        cppp::fixed_array<Value> values(fn.num_vals());
        const targets::rtl::Function::ip_t end = fn.instructions().end();
        for(targets::rtl::Function::ip_t ip = fn.instructions().begin();ip != end;++ip){
            const auto& ins = *ip;
            switch(ins.opcode){
                using enum targets::rtl::Operation;
                case CALL:
                    values[ins.dst] = call(values[ins.src].get<fptr>().id,values[ins.dst]);
                    break;
                case LDFN:
                    values[ins.dst] = fptr{ins.src};
                    break;    
                case LDI:
                    values[ins.dst] = uint32v{ins.src};
                    break;
                case ARG:
                    values[ins.dst] = arg;
                    break;
                case IPACK:
                    values[ins.dst] = values[ins.dst].get<pack>().values[ins.src];
                    break;
                case MKPACK:
                    values[ins.dst] = pack{};
                    break;
                case PACKATT:
                    values[ins.dst].get<pack>().values.emplace_back(values[ins.src]);
                    break;
                case ADD:
                    values[ins.dst].get<uint32v>().value += values[ins.src].get<uint32v>().value;
                    break;
                case SUB:
                    values[ins.dst].get<uint32v>().value -= values[ins.src].get<uint32v>().value;
                    break;
                case RET:
                    if(ins.dst == std::numeric_limits<std::uint32_t>::max()){
                        return {};
                    }else{
                        return values[ins.dst];
                    }
                case JMP:
                    ip = fn.get_label(ins.dst);
                    break;
                case JF:
                    if(!values[ins.src].get<boolv>().value){
                        ip = fn.get_label(ins.dst);
                    }
                    break;
                case MOV:
                    values[ins.dst] = values[ins.src];
                    break;
                case CEQ:
                    values[ins.dst] = boolv{values[ins.dst].get<uint32v>().value == values[ins.src].get<uint32v>().value};
                    break;
                case CLE:
                    values[ins.dst] = boolv{values[ins.dst].get<uint32v>().value <= values[ins.src].get<uint32v>().value};
                    break;
                case PRI:
                    std::cout << values[ins.dst].get<uint32v>().value << std::flush;
                    break;
                default:
                    throw std::logic_error("CompiledFunctionPool::call(): Unknown instruction "s+std::to_string(std::to_underlying(ins.opcode)));
            }
        }
        throw std::logic_error("CompiledFunctionPool::call(): fell off the end of the function without return"s);
    }
}
