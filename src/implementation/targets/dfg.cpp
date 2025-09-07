#include<bbe/targets/dfg.hpp>
#include<unordered_map>
#include<stdexcept>
#include<cassert>
#include<string>
namespace bbe::targets::dfg::impl{
    using namespace std::literals;
    class DfgCompiler{
        std::deque<DataNode>& nodes;
        std::unordered_map<std::uint32_t,const DataNode*> vars;
        DataNodeExecution ast_to_node(const ASTNode& nd,std::uint32_t type,const DataNode* se_after){
            DataNode& n = nodes.emplace_back(type);
            std::vector<const DataNode*> edep;
            for(const auto& c : nd.children()){
                DataNodeExecution res = compile_node(c,se_after);
                n.emplace(res.value());
                if(res.env()!=se_after){
                    edep.emplace_back(res.env());
                }
            }
            if(edep.empty()){
                return {&n,se_after};
            }else{
                edep.emplace_back(se_after);
                return {&n,&nodes.emplace_back(std::numeric_limits<std::uint32_t>::max(),std::move(edep))};
            }
        }
        DataNodeExecution compile_node(const ASTNode& nd,const DataNode* se_after){
            assert(se_after);
            switch(nd.type()){
                case 0: // u32
                    return {&nodes.emplace_back(0,nd.getp()),se_after};
                case 1: // u64
                    throw std::logic_error("dfg::compile_node(): Unsupported node type uint64"s);
                case 2:  // pack
                    return ast_to_node(nd,2,se_after);
                case 3: { // comma
                    const DataNode* ret;
                    for(std::size_t i=0uz;i<nd.children().size();++i){
                        DataNodeExecution ref = compile_node(nd.children()[i],se_after);
                        se_after = ref.env();
                        if(i==nd.getp()){
                            ret = ref.value();
                        }
                    }
                    return {ret,se_after};
                }
                case 5: // arg32
                    return {&nodes.emplace_back(5,nd.getp()),se_after};
                case 9: { // cmag
                    DataNode& n = nodes.emplace_back(9,nd.getp());
                    DataNodeExecution res = compile_node(nd.children().front(),se_after);
                    n.emplace(res.value());
                    const DataNode* envr;
                    switch(n.primitive()){
                        case 25: // pru32
                            n.emplace(res.env());
                            envr = &n;
                            break;
                        default:
                            envr = res.env();
                            break;
                    }
                    return {&n,envr};
                }
                case 10: { // setvar
                    DataNodeExecution res = compile_node(nd.children().front(),se_after);
                    vars.insert_or_assign(nd.getp(),res.value());
                    return res;
                }
                case 11: // getvar
                    return {vars.at(nd.getp()),se_after};
                case 21: // fork
                    return ast_to_node(nd,21,se_after);
                case 30: { // loopwhile
                    DataNode* lctrl = &nodes.emplace_back(301);
                    lctrl->emplace(se_after);
                    std::unordered_map<std::uint32_t,DataNode*> phin;
                    for(auto& [k,v] : vars){
                        DataNode* node = &nodes.emplace_back(300);
                        phin.try_emplace(k,node);
                        v = node;
                    }
                    DataNodeExecution body{compile_node(nd.children().front(),lctrl)};
                    for(auto& [k,v] : vars){
                        phin.at(k)->emplace(v);
                    }
                    lctrl->emplace(body.env());
                    return {nullptr,lctrl};
                }
                default:
                    throw std::logic_error("df::compile_node(): Unknown node type "s+std::to_string(nd.type()));
            }
        }
        public:
            DfgCompiler(std::deque<DataNode>& nodes) : nodes(nodes){}
            DataNodeExecution compile(const ASTNode& nd){
                return compile_node(nd,&nodes.emplace_back(std::numeric_limits<std::uint32_t>::max()));
            }
    };
    DataFlowGraph::DataFlowGraph(const Function& f) : _root(DfgCompiler(_nodes).compile(f.ast())){}
}
