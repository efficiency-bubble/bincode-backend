#include<bbe/inter/dfg.hpp>
#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<ranges>
#include<string>
#include<print>
namespace bbe::inter::dfg::impl{
    Value eval(const CompiledFunctionPool& pool,const targets::dfg::DataNode* nr,const Value& arg){
        switch(nr->operation()){
            using enum targets::dfg::NodeType;
            case UINT32:
                return uint32v{nr->primitive()};
            case SINT32:
                return sint32v{std::bit_cast<std::int32_t>(nr->primitive())};
            case PACK: {
                pack p;
                for(const targets::dfg::DataNode* ref : nr->parents()){
                    p.values.emplace_back(eval(pool,ref,arg));
                }
                return Value(std::move(p));
            }
            case PACKIND:
                return eval(pool,nr->parents().front(),arg).get<pack>().values[nr->primitive()];
            case ARG:
                return arg;
            case CALL_BUILTIN: {
                if(nr->primitive() == 0){ // call function
                    return pool.call(eval(pool,nr->parents()[0uz],arg).get<fptr>().id,eval(pool,nr->parents()[1uz],arg));
                }else{
                    std::vector<Value> values;
                    for(const targets::dfg::DataNode* par : nr->parents()){
                        values.emplace_back(eval(pool,par,arg));
                    }
                    return cmag(nr->primitive(),values);
                }
            }
            case BOOL:
                return boolv{nr->primitive()!=0};
            case FORK: {
                Value cond{eval(pool,nr->parents().front(),arg)};
                if(cond.get<boolv>().value){
                    return eval(pool,nr->parents()[1uz],arg);
                }else{
                    return eval(pool,nr->parents()[2uz],arg);
                }
            }
            case FNSYM:
                return fptr{nr->primitive()};
            case STDOUT:
                return {};
            default: throw std::logic_error("bbe::inter::dfg::eval(): Unknown node type "s+std::to_string(std::to_underlying(nr->operation())));
        }
    }
    Value CompiledFunctionPool::call(func_id fn,const Value& arg) const{
        return eval(*this,function(fn).root(),arg);
    }
}
