#include<bbe/targets/dfg.hpp>
#include<unordered_set>
#include<stdexcept>
#include<cassert>
#include<ranges>
#include<string>
namespace bbe::targets::dfg::impl{
    const DataNode* DataFlowGraph::compile(CodeBranch& br,const ASTNode& nd){
        switch(nd.type()){
            using enum NodeType;
            case UINT32:
                return &_nodes.emplace_back(0,nd.getp32());
            case UINT64:
                throw std::logic_error("dfg::compile(): Unsupported node type uint64"s);
            case PACK: {
                //TODO: customizable ordering (currently only sequential)
                DataNode* pack = &_nodes.emplace_back(2);
                for(const ASTNode& c : nd.children()){
                    pack->emplace(compile(br,c));
                }
                return pack;
            }
            case COMMA: {
                //TODO: customizable ordering (currently only sequential)
                const DataNode* result;
                std::uint32_t i = nd.getp32();
                for(const ASTNode& c : nd.children()){
                    const DataNode* n = compile(br,c);
                    if(!(i--)){
                        result = n;
                    }
                }
                // GCC please fix https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80922 so I don't have to -Wno-maybe-uninitialized
                return result;
            }
            case PACKIND:
                return &_nodes.emplace_back(4,nd.getp32(),std::vector{compile(br,nd.children().front())});
            case ARGV:
                return &_nodes.emplace_back(5);
            case CALL_BUILTIN: {
                DataNode& cmag = _nodes.emplace_back(9,nd.getp32());
                cmag.emplace(compile(br,nd.children().front()));
                switch(nd.getp32()){
                    case 25:
                        cmag.emplace(br.getvar(IO_VAR));
                        br.setvar(IO_VAR,&cmag);
                        break;
                    default:;
                }
                return &cmag;
            }
            case SETVAR: {
                const DataNode* res = compile(br,nd.children().front());
                br.setvar(nd.getp32(),res);
                return res;
            }
            case GETVAR:
                return br.getvar(nd.getp32());
            case BOOL: // bool
                return &_nodes.emplace_back(20,nd.getp32());
            case FORK: {
                const DataNode* condition = compile(br,nd.children().front());
                CodeBranch lcb{br};
                CodeBranch rcb{br};
                const DataNode* lhs = compile(lcb,nd.children()[1uz]);
                const DataNode* rhs = compile(rcb,nd.children()[2uz]);
                
                std::unordered_set<std::uint32_t> overrides;
                for(const auto& lv : lcb.local_vars()){
                    overrides.emplace(lv.first);
                }
                for(const auto& rv : rcb.local_vars()){
                    overrides.emplace(rv.first);
                }
                for(std::uint32_t v : overrides){
                    DataNode* fork = &_nodes.emplace_back(21);
                    fork->emplace(condition);
                    fork->emplace(lcb.getvar(v));
                    fork->emplace(rcb.getvar(v));
                    br.setvar(v,fork);
                }
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
    DataFlowGraph::DataFlowGraph(const Function& f) : _root((main.setvar(IO_VAR,&_nodes.emplace_back(400)),compile(main,f.ast()))){}
}
