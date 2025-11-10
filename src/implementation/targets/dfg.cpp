#include<bbe/targets/dfg.hpp>
#include<stdexcept>
#include<cassert>
#include<ranges>
#include<string>
namespace bbe::targets::dfg::impl{
    // Contract: only add one entry to clobbers
    const DataNode* DataFlowGraph::compile(const ASTNode& nd,Clobbers& clob){
        switch(nd.type()){
            case 0: // u32
                return &_nodes.emplace_back(0,nd.getp());
            case 1: // u64
                throw std::logic_error("dfg::compile(): Unsupported node type uint64"s);
            case 2: { // pack
                //TODO: customizable ordering (currently only parallel)
                Clobbers& sc = clob.then(false);
                DataNode* pack = &_nodes.emplace_back(2);
                for(const ASTNode& c : nd.children()){
                    pack->emplace(compile(c,sc));
                }
                return pack;
            }
            case 3: { // comma
                //TODO: customizable ordering (currently only sequential)
                Clobbers& sc = clob.then(true);
                const DataNode* result;
                std::size_t i = nd.getp();
                for(const ASTNode& c : nd.children()){
                    const DataNode* n = compile(c,sc);
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
                cmag->emplace(compile(nd.children().front(),clob));
                switch(nd.getp()){
                    case 25:
                        clob.push(cmag);
                        break;
                    default:;
                }
                return cmag;
            }
            case 10: { // setvar
                const DataNode* res = compile(nd.children().front(),clob);
                vars.insert_or_assign(static_cast<std::uint32_t>(nd.getp()),res);
                return res;
            }
            case 11: // getvar
                return vars.at(static_cast<std::uint32_t>(nd.getp()));
            case 20: // bool
                return &_nodes.emplace_back(20,nd.getp());
            case 21: { // fork
                const DataNode* condition = compile(nd.children().front(),clob);
                Fork& fk = clob.then_fork(condition);
                const DataNode* lhs = compile(nd.children()[1uz],fk.left());
                const DataNode* rhs = compile(nd.children()[2uz],fk.right());
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
                throw std::logic_error("DfgCompiler::compile(): Unknown node type "s+std::to_string(nd.type()));
        }
    }
    DataFlowGraph::DataFlowGraph(const Function& f) : _root(compile(f.ast(),clob)){}
}
