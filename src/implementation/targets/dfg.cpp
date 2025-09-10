#include<bbe/targets/dfg.hpp>
#include<unordered_map>
#include<stdexcept>
#include<string>
#include<limits>
namespace bbe::targets::dfg::impl{
    using namespace std::literals;
    class DfgCompiler{
        DataFlowGraph& graph;
        std::uint32_t stdoutc;
        std::unordered_map<std::uint32_t,const DataNode*> vars;
        template<std::size_t nc>
        const DataNode* ast_to_node(const ASTNode& nd,std::uint32_t type){
            DataNode& n = graph._nodes.emplace_back(type);
            [[assume(nc==std::numeric_limits<std::size_t>::max() || nc==nd.children().size())]];
            for(const auto& c : nd.children()){
                n.emplace(compile_node(c));
            }
            return &n;
        }
        public:
            DfgCompiler(DataFlowGraph& graph) : graph(graph){
                stdoutc = graph.clobberables.emplace();
            }
            const DataNode* compile_node(const ASTNode& nd){
                switch(nd.type()){
                    case 0: // u32
                        return &graph._nodes.emplace_back(0,nd.getp());
                    case 1: // u64
                        throw std::logic_error("dfg::compile_node(): Unsupported node type uint64"s);
                    case 2:  // pack
                        return ast_to_node<std::numeric_limits<std::size_t>::max()>(nd,2);
                    case 3: { // comma
                        const DataNode* ret;
                        for(std::size_t i=0uz;i<nd.children().size();++i){
                            const DataNode* ref = compile_node(nd.children()[i]);
                            if(i==nd.getp()){
                                ret = ref;
                            }
                        }
                        return ret;
                    }
                    case 5: // arg32
                        return &graph._nodes.emplace_back(5,nd.getp());
                    case 9: { // cmag
                        const DataNode* res = ast_to_node<2uz>(nd,9);
                        if(nd.getp()==25){ // print u32
                            graph.clobberables[stdoutc].sequence.emplace_back(res);
                        }
                        return res;
                    }
                    case 10: { // setvar
                        const DataNode* res = compile_node(nd.children().front());
                        vars.insert_or_assign(nd.getp(),res);
                        return res;
                    }
                    case 11: // getvar
                        return vars.at(nd.getp());
                    case 21: // fork
                        return ast_to_node<3>(nd,21);
                    case 200: // fn
                        return &graph._nodes.emplace_back(200,nd.getp());
                    default:
                        throw std::logic_error("DfgCompiler::compile_node(): Unknown node type "s+std::to_string(nd.type()));
                }
            }
    };
    DataFlowGraph::DataFlowGraph(const Function& f) : _root(DfgCompiler(*this).compile_node(f.ast())){}
}
