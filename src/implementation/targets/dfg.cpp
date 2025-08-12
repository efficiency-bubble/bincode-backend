#include<bbe/targets/dfg.hpp>
#include<unordered_map>
#include<stdexcept>
#include<string>
namespace bbe::targets::dfg::impl{
    using namespace std::literals;
    class DfgCompiler{
        std::deque<DataNode>& nodes;
        std::unordered_map<std::uint32_t,NodeRef> vars;
        public:
            DfgCompiler(std::deque<DataNode>& nodes) : nodes(nodes){}
            const DataNode& ast_to_node(const ASTNode& nd,std::uint32_t type){
                DataNode& n = nodes.emplace_back(type);
                for(const auto& c : nd.children()){
                    n.emplace(compile_node(c));
                }
                return n;
            }
            NodeRef compile_node(const ASTNode& nd){
                switch(nd.type()){
                    case 0: // u32
                        return nodes.emplace_back(0,nd.getp());
                    case 1: // u64
                        throw std::logic_error("dfg::compile_node(): Unsupported node type uint64"s);
                    case 2:  // pack
                        return ast_to_node(nd,2);
                    case 3: { // comma
                        NodeRef ret;
                        for(std::size_t i=0uz;i<nd.children().size();++i){
                            NodeRef ref = compile_node(nd.children()[i]);
                            if(i==nd.getp()){
                                ret = ref;
                            }
                        }
                        return ret;
                    }
                    case 5: // arg32
                        return nodes.emplace_back(5,nd.getp());
                    case 9: { // cmag
                        DataNode& n = nodes.emplace_back(9,nd.getp());
                        n.emplace(compile_node(nd.children().front()));
                        return n;
                    }
                    case 10: { // setvar
                        NodeRef value = compile_node(nd.children().front());
                        vars.insert_or_assign(nd.getp(),value);
                        return value;
                    }
                    case 11: // getvar
                        return vars.at(nd.getp());
                    case 21: // fork
                        return ast_to_node(nd,21);
                    default:
                        throw std::logic_error("df::compile_node(): Unknown node type "s+std::to_string(nd.type()));
                }
            }
    };
    DataFlowGraph::DataFlowGraph(const Function& f) : _root(DfgCompiler(_nodes).compile_node(f.ast())){}
}
