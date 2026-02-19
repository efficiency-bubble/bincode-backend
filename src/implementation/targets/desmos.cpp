#include<bbe/targets/desmos.hpp>
#include<unordered_map>
#include<stdexcept>
namespace bbe::targets::desmos::impl{
    namespace{
        class Compiler{
            cppp::sv prefix;
            std::unordered_map<const dfg::DataNode*,cppp::str> cache;
            cppp::str _compile_node(cppp::str& out,const dfg::DataNode* nd){
                switch(nd->operation()){
                    case 5: // argv
                        return u8"a_{rgv}"s;
                    case 0: // uint32
                    case 20: { // bool
                        return cppp::tou8(std::to_string(nd->primitive()));
                    }
                    case 21: { // fork
                        cppp::str cond{compile_node(out,nd->parents().front())};
                        cppp::str lhs{compile_node(out,nd->parents()[1uz])};
                        cppp::str rhs{compile_node(out,nd->parents()[2uz])};
                        return u8"\\left\\{"sv+cond+u8"=1:"sv+lhs+u8','+rhs+u8"\\right\\}\n"sv;
                    }
                    default:
                        throw std::logic_error("Desmos compile: unknown node type "s+std::to_string(nd->operation()));
                }
            }
            cppp::str compile_node(cppp::str& out,const dfg::DataNode* nd){
                if(auto it=cache.find(nd);it!=cache.end()){
                    return it->second;
                }else{
                    return cache.try_emplace(nd,_compile_node(out,nd)).first->second;
                }
            }
            public:
                Compiler(cppp::sv prefix) : prefix(prefix){}
                void compile(cppp::str& out,const dfg::DataNode* nd){
                    cppp::str ret{compile_node(out,nd)};
                    out.append(u8"f_{"sv);
                    out.append(prefix);
                    out.append(u8"}(a_{rgv}) = "sv);
                    out.append(ret);
                }
        };
    }
    void compile(cppp::str& out,const dfg::DataFlowGraph& fn,cppp::sv prefix){
        Compiler(prefix).compile(out,fn.root());
    }
}
