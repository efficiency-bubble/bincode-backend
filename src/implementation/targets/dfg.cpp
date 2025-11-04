#include<bbe/targets/dfg.hpp>
#include<stdexcept>
#include<cassert>
#include<ranges>
#include<string>
namespace bbe::targets::dfg::impl{
    const DataNode* DataFlowGraph::compile_node(const ASTNode& nd,obs_t& observables){
        switch(nd.type()){
            case 0: // u32
                return &_nodes.emplace_back(0,nd.getp());
            case 1: // u64
                throw std::logic_error("dfg::compile_node(): Unsupported node type uint64"s);
            case 2: { // pack
                DataNode* pack = &_nodes.emplace_back(2);
                std::vector<obs_t> cobs;
                for(const ASTNode& c : nd.children()){
                    pack->emplace(compile_node(c,cobs.emplace_back(observables)));
                }
                for(std::size_t i=0;i<observables.size();++i){
                    DataNode& seq_pt = _nodes.emplace_back(350);
                    for(const auto& obs : cobs){
                        seq_pt.emplace(obs[i]);
                    }
                    observables[i] = &seq_pt;
                }
                return pack;
            }
            case 3: { // comma
                const DataNode* result;
                std::size_t i = nd.getp();
                for(const ASTNode& c : nd.children()){
                    const DataNode* n = compile_node(c,observables);
                    if(!(i--)){
                        result = n;
                    }
                }
                return result;
            }
            case 5: // arg32
                return &_nodes.emplace_back(5,nd.getp());
            case 9: { // cmag
                DataNode* cmag = &_nodes.emplace_back(9,nd.getp());
                cmag->emplace(compile_node(nd.children().front(),observables));
                cmag->emplace(observables.front());
                observables.front() = cmag;
                return cmag;
            }
            case 10: { // setvar
                const DataNode* res = compile_node(nd.children().front(),observables);
                vars.insert_or_assign(static_cast<std::uint32_t>(nd.getp()),res);
                return res;
            }
            case 11: // getvar
                return vars.at(static_cast<std::uint32_t>(nd.getp()));
            case 20: // bool
                return &_nodes.emplace_back(20,nd.getp());
            case 21: { // fork
                const DataNode* condition = compile_node(nd.children().front(),observables);
                obs_t lho{observables};
                const DataNode* lhs = compile_node(nd.children()[1uz],lho);
                obs_t rho{observables};
                const DataNode* rhs = compile_node(nd.children()[2uz],rho);
                for(std::size_t i=0;i<observables.size();++i){
                    DataNode& jobs = _nodes.emplace_back(21);
                    jobs.emplace(condition);
                    jobs.emplace(lho[i]);
                    jobs.emplace(rho[i]);
                    observables[i] = &jobs;
                }
                DataNode* join = &_nodes.emplace_back(21);
                join->emplace(condition);
                join->emplace(lhs);
                join->emplace(rhs);
                return join;
            }
            case 30: // loopwhile
                assert(false); // TODO
            case 200: // fn
                return &_nodes.emplace_back(200,nd.getp());
            default:
                throw std::logic_error("DfgCompiler::compile_node(): Unknown node type "s+std::to_string(nd.type()));
        }
    }
    const DataNode* DataFlowGraph::compile(const ASTNode& nd){
        observables.emplace_back(&_nodes.emplace_back(std::numeric_limits<std::uint32_t>::max()));
        return compile_node(nd,observables);
    }
    DataFlowGraph::DataFlowGraph(const Function& f) : _root(compile(f.ast())){}
}
