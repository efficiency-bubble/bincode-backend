#include<bbe/inter/dfg.hpp>
#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<ranges>
#include<string>
#include<print>
namespace bbe::inter::dfg::impl{
    static Value dedup_eval(const CompiledFunctionPool& pool,const targets::dfg::DataNode& nr,const Value& arg,std::unordered_map<const targets::dfg::DataNode*,Value>& cache);
    static Value eval(const CompiledFunctionPool& pool,const targets::dfg::DataNode& nr,const Value& arg,std::unordered_map<const targets::dfg::DataNode*,Value>& cache){
        switch(nr.operation()){
            using enum targets::dfg::NodeType;
            case UINT32:
                return uint32v{nr.primitive()};
            case SINT32:
                return sint32v{std::bit_cast<std::int32_t>(nr.primitive())};
            case PACK: {
                pack p;
                for(const targets::dfg::DataNode* ref : nr.parents()){
                    p.values.emplace_back(dedup_eval(pool,*ref,arg,cache));
                }
                return Value(std::move(p));
            }
            case PACKIND:
                return dedup_eval(pool,*nr.parents().front(),arg,cache).get<pack>().values[nr.primitive()];
            case ARG:
                return arg;
            case CALL_BUILTIN: {
                if(nr.primitive() == 0){ // call function
                    return pool.call(dedup_eval(pool,*nr.parents()[0uz],arg,cache).get<fptr>().id,dedup_eval(pool,*nr.parents()[1uz],arg,cache));
                }else{
                    std::vector<Value> values;
                    for(const targets::dfg::DataNode* par : nr.parents()){
                        values.emplace_back(dedup_eval(pool,*par,arg,cache));
                    }
                    return cmag(nr.primitive(),values);
                }
            }
            case BOOL:
                return boolv{nr.primitive()!=0};
            case FORK: {
                Value cond{dedup_eval(pool,*nr.parents()[0uz],arg,cache)};
                if(cond.get<boolv>().value){
                    return dedup_eval(pool,*nr.parents()[1uz],arg,cache);
                }else{
                    return dedup_eval(pool,*nr.parents()[2uz],arg,cache);
                }
            }
            case FNSYM:
                return fptr{nr.primitive()};
            case SEQU:
                for(const targets::dfg::DataNode* par : nr.parents()){
                    dedup_eval(pool,*par,arg,cache);
                }
                [[fallthrough]];
            case DUMMY:
                return {};
            default: throw std::logic_error("bbe::inter::dfg::eval(): Unknown node type "s+std::to_string(std::to_underlying(nr.operation())));
        }
    }
    static Value dedup_eval(const CompiledFunctionPool& pool,const targets::dfg::DataNode& nr,const Value& arg,std::unordered_map<const targets::dfg::DataNode*,Value>& cache){
        if(auto it=cache.find(&nr);it!=cache.end()){
            return it->second;
        }
        return cache.try_emplace(&nr,eval(pool,nr,arg,cache)).first->second;
    }
    Value CompiledFunctionPool::call(func_id fn,const Value& arg) const{
        targets::dfg::Operation op{function(fn).root()};
        std::unordered_map<const targets::dfg::DataNode*,Value> cache;
        if(op.side_effects()) dedup_eval(*this,*op.side_effects(),arg,cache);
        return dedup_eval(*this,op.value(),arg,cache);
    }
}
