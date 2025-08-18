#include<bbe/inter/dfg.hpp>
#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<string>
namespace bbe::inter::dfg::impl{
    using namespace bbe::inter::impl;
    using namespace std::literals;
    Value eval(FunctionCall& call,const targets::dfg::DataNode* nr){
        switch(nr->operation()){
            case 0: // u32
                return uint32v{nr->primitive()};
            case 2: { // pack
                pack p;
                for(const auto& ref : nr->parents()){
                    p.values.emplace_back(eval(call,ref));
                }
                return Value(std::move(p));
            }
            case 5: // arg32
                return call.argv[nr->primitive()];
            case 9: // cmag
                if(nr->parents().size()>1uz){
                    eval(call,nr->parents()[1uz]); // side effect dependency
                }
                return cmag(nr->primitive(),eval(call,nr->parents().front()));
            case 21: { // fork
                Value cond{eval(call,nr->parents().front())};
                if(cond.get<boolv>().value){
                    return eval(call,nr->parents()[1uz]);
                }else{
                    return eval(call,nr->parents()[2uz]);
                }
            }
            case std::numeric_limits<std::uint32_t>::max(): return {}; // env
            default: throw std::logic_error("bbe::inter::dfg::eval(): Unknown node type"s);
        }
    }
}
