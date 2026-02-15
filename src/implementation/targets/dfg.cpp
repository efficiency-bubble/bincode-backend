#include<bbe/targets/dfg.hpp>
#include<stdexcept>
#include<cassert>
#include<ranges>
#include<string>
namespace bbe::targets::dfg::impl{
    // Contract: only add one entry to clobbers
    const DataNode* DataFlowGraph::compile(const ASTNode& nd,Clobbers& clob){
        switch(nd.type()){
            using enum NodeType;
            case UINT32:
                return &_nodes.emplace_back(0,nd.getp32());
            case UINT64:
                throw std::logic_error("dfg::compile(): Unsupported node type uint64"s);
            case PACK: {
                //TODO: customizable ordering (currently only parallel)
                Clobbers& sc = clob.then(false);
                DataNode* pack = &_nodes.emplace_back(2);
                for(const ASTNode& c : nd.children()){
                    pack->emplace(compile(c,sc));
                }
                return pack;
            }
            case COMMA: {
                //TODO: customizable ordering (currently only sequential)
                Clobbers& sc = clob.then(true);
                const DataNode* result;
                std::uint32_t i = nd.getp32();
                for(const ASTNode& c : nd.children()){
                    const DataNode* n = compile(c,sc);
                    if(!(i--)){
                        result = n;
                    }
                }
                // GCC please fix https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80922 so I don't have to -Wno-maybe-uninitialized
                return result;
            }
            case ARGV:
                return &_nodes.emplace_back(5);
            case CALL_BUILTIN: {
                Clobbers& sc = clob.then(true);
                DataNode* cmag = &_nodes.emplace_back(9,nd.getp32());
                cmag->emplace(compile(nd.children().front(),sc));
                switch(nd.getp32()){
                    case 25:
                        sc.push(cmag);
                        break;
                    default:;
                }
                return cmag;
            }
            case SETVAR: {
                const DataNode* res = compile(nd.children().front(),clob);
                vars.insert_or_assign(nd.getp32(),res);
                return res;
            }
            case GETVAR:
                return vars.at(nd.getp32());
            case BOOL: // bool
                return &_nodes.emplace_back(20,nd.getp32());
            case FORK: {
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
            case FOREVER:
                assert(false); // TODO
            case FNSYM:
                return &_nodes.emplace_back(200,nd.getp32());
            default:
                throw std::logic_error("DfgCompiler::compile(): Unknown node type "s+std::to_string(std::to_underlying(nd.type())));
        }
    }
    DataFlowGraph::DataFlowGraph(const Function& f) : _root(compile(f.ast(),clob)){}
}
