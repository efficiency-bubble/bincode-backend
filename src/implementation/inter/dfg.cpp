#include<bbe/inter/dfg.hpp>
#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<string>
namespace bbe::inter::dfg::impl{
    using namespace bbe::inter::impl;
    using namespace std::literals;
    Value eval(FunctionCall& call,targets::dfg::NodeRef nr){
        const targets::dfg::DataNode& nd = nr.node();
        switch(nd.operation()){
            case 0: // u32
                return uint32v{nd.primitive()};
            case 2: { // pack
                pack p;
                for(const auto& ref : nd.parents()){
                    p.values.emplace_back(eval(call,ref));
                }
                return Value(std::move(p));
            }
            case 5: // arg32
                return call.argv[nd.primitive()];
            case 9: // cmag
                return cmag(nd.primitive(),eval(call,nd.parents().front()));
            case 21: { // fork
                Value cond{eval(call,nd.parents().front())};
                if(cond.get<boolv>().value){
                    return eval(call,nd.parents()[1uz]);
                }else{
                    return eval(call,nd.parents()[2uz]);
                }
            }
            default: throw std::logic_error("bbe::inter::dfg::eval(): Unknown node type"s);
        }
    }
}
