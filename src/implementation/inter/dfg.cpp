#include<bbe/inter/dfg.hpp>
#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<string>
#include<print>
namespace bbe::inter::dfg::impl{
    Value eval(const CompiledFunctionPool& pool,const targets::dfg::DataNode* nr,const std::vector<Value>& argv){
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
                return pack{argv};
            case CALL_BUILTIN:
                if(nr->primitive() == 0){ // call function
                    return pool.call(eval(pool,nr->parents()[0uz],argv).get<fptr>().id,eval(pool,nr->parents()[1uz],argv).get<pack>().values);
                }else{
                    return cmag(nr->primitive(),eval(pool,nr->parents().front(),argv));
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
    Value CompiledFunctionPool::call(ProjectEntitiesPool::index_type fn,const std::vector<Value>& argv) const{
        return eval(*this,function(fn).root(),argv);
    }
}
