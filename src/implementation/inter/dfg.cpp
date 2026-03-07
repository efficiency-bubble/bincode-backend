#include<bbe/inter/dfg.hpp>
#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<string>
#include<print>
namespace bbe::inter::dfg::impl{
    Value eval(const CompiledFunctionPool& pool,const targets::dfg::DataNode* nr,const std::vector<Value>& argv){
        switch(nr->operation()){
            case 0: // u32
                return uint32v{nr->primitive()};
            case 2: { // pack
                pack p;
                for(const auto& ref : nr->parents()){
                    p.values.emplace_back(eval(pool,ref,argv));
                }
                return Value(std::move(p));
            }
            case 4: // packind
                return eval(pool,nr->parents().front(),argv).get<pack>().values[nr->primitive()];
            case 5: // argv
                return pack{argv};
            case 9: // cmag
                if(nr->primitive() == 0){ // call function
                    return pool.call(eval(pool,nr->parents()[0uz],argv).get<fptr>().id,eval(pool,nr->parents()[1uz],argv).get<pack>().values);
                }else{
                    return cmag(nr->primitive(),eval(pool,nr->parents().front(),argv));
                }
            case 20: // bool
                return boolv{nr->primitive()!=0};
            case 21: { // fork
                Value cond{eval(pool,nr->parents().front(),argv)};
                if(cond.get<boolv>().value){
                    return eval(pool,nr->parents()[1uz],argv);
                }else{
                    return eval(pool,nr->parents()[2uz],argv);
                }
            }
            case 200: // fn
                return fptr{nr->primitive()};
            case 301: { // lctrl
                while(true){
                    eval(pool,nr->parents()[1uz],argv);
                }
            }
            case std::numeric_limits<std::uint32_t>::max(): return {}; // env
            default: throw std::logic_error("bbe::inter::dfg::eval(): Unknown node type "s+std::to_string(nr->operation()));
        }
    }
    Value CompiledFunctionPool::call(ProjectEntitiesPool::index_type fn,const std::vector<Value>& argv) const{
        return eval(*this,function(fn).root(),argv);
    }
}
