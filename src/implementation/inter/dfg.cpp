#include<bbe/inter/dfg.hpp>
#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<ranges>
#include<string>
#include<print>
namespace bbe::inter::dfg::impl{
    Value eval(const CompiledFunctionPool& pool,const targets::dfg::DataNode* nr,cppp::view<const Value> argv){
        switch(nr->operation()){
            using enum targets::dfg::NodeType;
            case UINT32:
                return uint32v{nr->primitive()};
            case PACK: {
                pack p;
                for(const targets::dfg::DataNode* ref : nr->parents()){
                    p.values.emplace_back(eval(pool,ref,argv));
                }
                return Value(std::move(p));
            }
            case PACKIND:
                return eval(pool,nr->parents().front(),argv).get<pack>().values[nr->primitive()];
            case ARGV:
                return pack{std::vector<Value>(std::from_range,argv)};
            case CALL_BUILTIN: {
                std::vector<Value> values;
                for(const targets::dfg::DataNode* par : nr->parents()){
                    values.emplace_back(eval(pool,par,argv));
                }
                if(nr->primitive() == 0){ // call function
                    return pool.call(values.front().get<fptr>().id,cppp::view(values).slice(1uz));
                }else{
                    return cmag(nr->primitive(),values);
                }
            }
            case BOOL:
                return boolv{nr->primitive()!=0};
            case FORK: {
                Value cond{eval(pool,nr->parents().front(),argv)};
                if(cond.get<boolv>().value){
                    return eval(pool,nr->parents()[1uz],argv);
                }else{
                    return eval(pool,nr->parents()[2uz],argv);
                }
            }
            case FNSYM:
                return fptr{nr->primitive()};
            default: throw std::logic_error("bbe::inter::dfg::eval(): Unknown node type "s+std::to_string(std::to_underlying(nr->operation())));
        }
    }
    Value CompiledFunctionPool::call(ProjectEntitiesPool::index_type fn,cppp::view<const Value> argv) const{
        return eval(*this,function(fn).root(),argv);
    }
}
