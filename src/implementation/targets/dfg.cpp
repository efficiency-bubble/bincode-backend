#include<bbe/targets/dfg.hpp>
#include<unordered_set>
#include<stdexcept>
#include<cassert>
#include<ranges>
#include<string>
namespace bbe::targets::dfg::impl{
    using namespace std::literals;
    static const DataNode* se_merge(std::deque<DataNode>& nodes,const DataNode* lse,const DataNode* rse){
        if(lse && rse){
            DataNode& seq = nodes.emplace_back(NodeType::SEQU,TypeDatabase::T_ERROR);
            seq.emplace(*lse);
            seq.emplace(*rse);
            return &seq;
        }else if(lse){
            return lse;
        }else{
            return rse;
        }
    }
    Operation DataFlowGraph::compile(CodeBranch& br,const ASTNode& nd){
        switch(nd.type()){
            using enum bbe::NodeType;
            case UINT32:
                return _nodes.emplace_back(NodeType::UINT32,nd.result_type(),nd.getp32());
            case SINT32:
                return _nodes.emplace_back(NodeType::SINT32,nd.result_type(),nd.getp32());
            case UINT64:
                throw std::logic_error("dfg::compile(): Unsupported node type uint64"s);
            case PACK: {
                DataNode& pack = _nodes.emplace_back(NodeType::PACK,nd.result_type());
                const DataNode* se = nullptr;
                for(const ASTNode& c : nd.children()){
                    Operation op{compile(br,c)};
                    if(const DataNode* ese = op.side_effects()){
                        if(se) throw std::logic_error("Side effects are indeterminately ordered");
                        se = ese;
                    }
                    pack.emplace(op.value());
                }
                return {pack,se};
            }
            case COMMA: {
                //TODO: customizable ordering (currently only sequential)
                const DataNode* result;
                DataNode* se = nullptr;
                std::uint32_t i = nd.getp32();
                for(const ASTNode& c : nd.children()){
                    Operation op{compile(br,c)};
                    if(!(i--)){
                        result = &op.value();
                    }
                    if(op.side_effects()){
                        if(!se) se = &_nodes.emplace_back(NodeType::SEQU,TypeDatabase::T_ERROR);
                        se->emplace(op.value());
                    }
                }
                // GCC please fix https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80922 so I don't have to -Wno-maybe-uninitialized
                return {*result,se};
            }
            case PACKIND: {
                Operation op{compile(br,nd.children().front())};
                return {_nodes.emplace_back(NodeType::PACKIND,nd.result_type(),nd.getp32(),std::vector{&op.value()}),op.side_effects()};
            }
            case ARG:
                return _nodes.emplace_back(NodeType::ARG,nd.result_type());
            case CALL_BUILTIN: {
                std::uint32_t fnid = nd.getp32();
                bool side_effects = false;
                switch(fnid){
                    case 0:
                        side_effects = true;
                        break;
                    case 10:
                        switch(nd.result_type()){
                            case TypeDatabase::T_UINT32:
                                // fnid = 10; // already 10
                                break;
                            case TypeDatabase::T_INT32:
                                fnid = 11;
                                break;
                            default: throw std::logic_error("DataFlowGraph::compile(): Don't know what kind of addition results in type "s+std::to_string(nd.result_type()));
                        }
                        break;
                    case 20:
                        switch(nd.result_type()){
                            case TypeDatabase::T_UINT32:
                                // fnid = 20; // already 20
                                break;
                            case TypeDatabase::T_INT32:
                                fnid = 21;
                                break;
                            default: throw std::logic_error("DataFlowGraph::compile(): Don't know what kind of subtraction results in type "s+std::to_string(nd.result_type()));
                        }
                        break;
                    case 30:
                        switch(nd.result_type()){
                            case TypeDatabase::T_UINT32:
                                // fnid = 30; // already 30
                                break;
                            case TypeDatabase::T_INT32:
                                fnid = 31;
                                break;
                            default: throw std::logic_error("DataFlowGraph::compile(): Don't know what kind of multiplication results in type "s+std::to_string(nd.result_type()));
                        }
                        break;
                    case 100:
                        side_effects = true;
                        break;
                    default:;
                }
                DataNode& cmag = _nodes.emplace_back(NodeType::CALL_BUILTIN,nd.result_type(),fnid);
                const DataNode* se = nullptr;
                for(const ASTNode& c : nd.children()){
                    Operation op{compile(br,c)};
                    cmag.emplace(op.value());
                    if(const DataNode* ese = op.side_effects()){
                        if(se) throw std::logic_error("Side effects are indeterminately ordered");
                        se = ese;
                    }
                }
                return {cmag,se_merge(_nodes,side_effects?&cmag:nullptr,se)};
            }
            case SETVAR: {
                Operation op{compile(br,nd.children().front())};
                br.setvar(nd.getp32(),op.value());
                return {_nodes.emplace_back(NodeType::VOID,TypeDatabase::T_VOID),op.side_effects()};
            }
            case GETVAR:
                return *br.getvar(nd.getp32());
            case HAVEVAR: {
                CodeBranch local_scope{br};
                Operation vop{compile(br,nd.children()[0uz])};
                local_scope.setvar(nd.getp32(),vop.value());
                Operation eop{compile(local_scope,nd.children()[1uz])};
                return {eop.value(),se_merge(_nodes,vop.side_effects(),eop.side_effects())};
            }
            case BOOL: // bool
                return _nodes.emplace_back(NodeType::BOOL,nd.result_type(),nd.getp32());
            case FORK: {
                Operation condition{compile(br,nd.children().front())};
                CodeBranch lcb{br};
                CodeBranch rcb{br};
                Operation lhs{compile(lcb,nd.children()[1uz])};
                Operation rhs{compile(rcb,nd.children()[2uz])};
                
                std::unordered_set<std::uint32_t> overrides;
                for(const auto& lv : lcb.local_vars()){
                    overrides.emplace(lv.first);
                }
                for(const auto& rv : rcb.local_vars()){
                    overrides.emplace(rv.first);
                }
                for(std::uint32_t v : overrides){
                    DataNode& fork = _nodes.emplace_back(NodeType::FORK,br.getvar(v)->return_type());
                    fork.emplace(condition.value());
                    fork.emplace(*lcb.getvar(v));
                    fork.emplace(*rcb.getvar(v));
                    br.setvar(v,fork);
                }
                DataNode& join = _nodes.emplace_back(NodeType::FORK,nd.result_type());
                join.emplace(condition.value());
                join.emplace(lhs.value());
                join.emplace(rhs.value());
                if(condition.side_effects() || lhs.side_effects() || rhs.side_effects()){
                    DataNode& sejoin = _nodes.emplace_back(NodeType::FORK,TypeDatabase::T_VOID);
                    sejoin.emplace(condition.value());
                    if(const DataNode* lp = se_merge(_nodes,condition.side_effects(),lhs.side_effects())){
                        sejoin.emplace(*lp);
                    }else{
                        sejoin.emplace(_nodes.emplace_back(NodeType::DUMMY,TypeDatabase::T_VOID));
                    }
                    if(const DataNode* rp = se_merge(_nodes,condition.side_effects(),rhs.side_effects())){
                        sejoin.emplace(*rp);
                    }else{
                        sejoin.emplace(_nodes.emplace_back(NodeType::DUMMY,TypeDatabase::T_VOID));
                    }
                    return {join,&sejoin};
                }else{
                    return join;
                }
            }
            case FNSYM:
                return _nodes.emplace_back(NodeType::FNSYM,nd.result_type(),nd.getp32());
            case IMPORT_STUB:
                cppp::unreachable();
            case UINT32SYM:
            case NTYPE:
                throw std::logic_error("DataFlowGraph::compile(): Unexpected node type "s+std::to_string(std::to_underlying(nd.type())));
        }
        cppp::unreachable();
    }
    DataFlowGraph::DataFlowGraph(const bbe::Function& f) : _root(compile(main,f.ast())){}
}
