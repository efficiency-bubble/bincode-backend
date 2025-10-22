#include<bbe/targets/dfg.hpp>
#include<unordered_map>
#include<stdexcept>
#include<cassert>
#include<ranges>
#include<string>
namespace bbe::targets::dfg::impl{
    using namespace std::literals;
    class DfgCompiler{
        std::deque<DataNode>& nodes;
        std::unordered_map<std::uint32_t,const DataNode*> vars;
        constexpr static std::uint32_t STDOUT_V = 0;
        const DataNode* compile_node(const ASTNode& nd,const DataNode*& envp){
            switch(nd.type()){
                case 0: // u32
                    return &nodes.emplace_back(0,nd.getp());
                case 1: // u64
                    throw std::logic_error("dfg::compile_node(): Unsupported node type uint64"s);
                case 2: { // pack
                    DataNode* pack = &nodes.emplace_back(2);
                    DataNode seq_pt{350};
                    bool has_side_effects = false;
                    for(const ASTNode& c : nd.children()){
                        const DataNode* ep2 = envp;
                        pack->emplace(compile_node(c,ep2));
                        if(ep2 != envp){
                            seq_pt.emplace(ep2);
                            has_side_effects = true;
                        }
                    }
                    if(has_side_effects){
                        envp = &nodes.emplace_back(std::move(seq_pt));
                    }
                    return pack;
                }
                case 3: { // comma
                    const DataNode* result;
                    std::size_t i = nd.getp();
                    for(const ASTNode& c : nd.children()){
                        const DataNode* n = compile_node(c,envp);
                        if(!(i--)){
                            result = n;
                        }
                    }
                    return result;
                }
                case 5: // arg32
                    return &nodes.emplace_back(5,nd.getp());
                case 9: { // cmag
                    DataNode* cmag = &nodes.emplace_back(9,nd.getp());
                    cmag->emplace(compile_node(nd.children().front(),envp));
                    cmag->emplace(envp);
                    envp = cmag;
                    return cmag;
                }
                case 10: { // setvar
                    const DataNode* res = compile_node(nd.children().front(),envp);
                    vars.insert_or_assign(static_cast<std::uint32_t>(nd.getp()),res);
                    return res;
                }
                case 11: // getvar
                    return vars.at(static_cast<std::uint32_t>(nd.getp()));
                case 21: // fork
                    assert(false); // TODO
                case 30: // loopwhile
                    assert(false); // TODO
                case 200: // fn
                    return &nodes.emplace_back(200,nd.getp());
                default:
                    throw std::logic_error("DfgCompiler::compile_node(): Unknown node type "s+std::to_string(nd.type()));
            }
        }
        public:
            DfgCompiler(std::deque<DataNode>& nodes) : nodes(nodes){}
            const DataNode* compile(const ASTNode& nd){
                const DataNode* envp = &nodes.emplace_back(std::numeric_limits<std::uint32_t>::max());
                vars.try_emplace(STDOUT_V,envp);
                return compile_node(nd,envp);
            }
    };
    DataFlowGraph::DataFlowGraph(const Function& f) : _root(DfgCompiler(_nodes).compile(f.ast())){}
}
