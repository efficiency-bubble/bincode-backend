#include<bbe/targets/dfg.hpp>
#include<stdexcept>
#include<string>
namespace bbe::targets::dfg::impl{
    using namespace std::literals;
    NodeRef compile_node(std::deque<DataNode>&,const ASTNode&);
    const DataNode& ast_to_node(std::deque<DataNode>& nodes,const ASTNode& nd,std::uint32_t type){
        DataNode& n = nodes.emplace_back(type);
        for(const auto& c : nd.children()){
            n.emplace(compile_node(nodes,c));
        }
        return n;
    }
    NodeRef compile_node(std::deque<DataNode>& nodes,const ASTNode& nd){
        switch(nd.type()){
            case 0: // u32
                return nodes.emplace_back(0,nd.getp());
            case 1: // u64
                throw std::logic_error("dfg::compile_node(): Unsupported node type uint64"s);
            case 2:  // pack
                return ast_to_node(nodes,nd,2);
            case 5: // arg32
                return nodes.emplace_back(5,nd.getp());
            case 9: { // cmag
                DataNode& n = nodes.emplace_back(9,nd.getp());
                n.emplace(compile_node(nodes,nd.children().front()));
                return n;
            }
            case 21: // fork
                return ast_to_node(nodes,nd,21);
            default:
                throw std::logic_error("df::compile_node(): Unknown node type "s+std::to_string(nd.type()));
        }
    }
    DataFlowGraph::DataFlowGraph(const Function& f) : _root(compile_node(_nodes,f.ast())){}
}
