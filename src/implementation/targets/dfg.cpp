#include<bbe/targets/dfg.hpp>
#include<cppp/assert.hpp>
#include<unordered_set>
#include<stdexcept>
#include<cassert>
#include<ranges>
#include<string>
namespace bbe::targets::dfg::impl{
    static const DataNode* se_merge(std::deque<DataNode>& nodes,const DataNode* lse,const DataNode* rse){
        if(lse && rse){
            DataNode& seq = nodes.emplace_back(NodeType::SEQU,T_ERROR);
            seq.emplace(*lse);
            seq.emplace(*rse);
            return &seq;
        }else if(lse){
            return lse;
        }else{
            return rse;
        }
    }
    Operation DataFlowGraph::compile(const ProjectEntitiesPool& pep,CodeBranch& br,const ASTNode& nd){
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
                    Operation op{compile(pep,br,c)};
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
                    Operation op{compile(pep,br,c)};
                    if(!(i--)){
                        result = &op.value();
                    }
                    if(op.side_effects()){
                        if(!se) se = &_nodes.emplace_back(NodeType::SEQU,T_ERROR);
                        se->emplace(op.value());
                    }
                }
                // GCC please fix https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80922 so I don't have to -Wno-maybe-uninitialized
                return {*result,se};
            }
            case PACKIND: {
                Operation op{compile(pep,br,nd.children().front())};
                return {_nodes.emplace_back(NodeType::PACKIND,nd.result_type(),nd.getp32(),std::vector{&op.value()}),op.side_effects()};
            }
            case ARG:
                return _nodes.emplace_back(NodeType::ARG,nd.result_type(),nd.getp32());
            case DEREF: {
                Operation op{compile(pep,br,nd.children().front())};
                return {_nodes.emplace_back(NodeType::DEREF,nd.result_type(),std::vector{&op.value()}),op.side_effects()};
            }
            case ADDROF: {
                Operation op{compile(pep,br,nd.children().front())};
                return {_nodes.emplace_back(NodeType::ADDROF,nd.result_type(),std::vector{&op.value()}),op.side_effects()};
            }
            case CALL: {
                bool side_effects = false;
                std::uint32_t fnid = std::numeric_limits<std::uint32_t>::max();
                if(nd.children()[0].type() == bbe::NodeType::FNSYM){
                    if(const bbe::Function& fn = pep.functions()[nd.children()[0].getp32()];fn.is_intrin()){
                        fnid = fn.intrin();
                    }
                }
                DataNode& cmag = _nodes.emplace_back(NodeType::CALL_BUILTIN,nd.result_type(),fnid);
                const DataNode* se = nullptr;
                for(const ASTNode& c : nd.children() | std::views::drop(fnid != std::numeric_limits<std::uint32_t>::max())){
                    Operation op{compile(pep,br,c)};
                    cmag.emplace(op.value());
                    if(const DataNode* ese = op.side_effects()){
                        if(se) throw std::logic_error("Side effects are indeterminately ordered");
                        se = ese;
                    }
                }
                return {cmag,se_merge(_nodes,side_effects?&cmag:nullptr,se)};
            }
            case SETVAR: {
                Operation op{compile(pep,br,nd.children().front())};
                br.setvar(nd.getp32(),op.value());
                return {_nodes.emplace_back(NodeType::VOID,T_VOID),op.side_effects()};
            }
            case GETVAR:
                return *br.getvar(nd.getp32());
            case HAVEVAR: {
                CodeBranch local_scope{br};
                Operation vop{compile(pep,br,nd.children()[0_u32])};
                local_scope.setvar(nd.getp32(),vop.value());
                Operation eop{compile(pep,local_scope,nd.children()[1_u32])};
                return {eop.value(),se_merge(_nodes,vop.side_effects(),eop.side_effects())};
            }
            case BOOL: // bool
                return _nodes.emplace_back(NodeType::BOOL,nd.result_type(),nd.getp32());
            case FORK: {
                Operation condition{compile(pep,br,nd.children().front())};
                CodeBranch lcb{br};
                CodeBranch rcb{br};
                Operation lhs{compile(pep,lcb,nd.children()[1_u32])};
                Operation rhs{compile(pep,rcb,nd.children()[2_u32])};
                
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
                    DataNode& sejoin = _nodes.emplace_back(NodeType::FORK,T_VOID);
                    sejoin.emplace(condition.value());
                    if(const DataNode* lp = se_merge(_nodes,condition.side_effects(),lhs.side_effects())){
                        sejoin.emplace(*lp);
                    }else{
                        sejoin.emplace(_nodes.emplace_back(NodeType::DUMMY,T_VOID));
                    }
                    if(const DataNode* rp = se_merge(_nodes,condition.side_effects(),rhs.side_effects())){
                        sejoin.emplace(*rp);
                    }else{
                        sejoin.emplace(_nodes.emplace_back(NodeType::DUMMY,T_VOID));
                    }
                    return {join,&sejoin};
                }else{
                    return join;
                }
            }
            case FNSYM:
                return _nodes.emplace_back(NodeType::FNSYM,nd.result_type(),nd.getp32());
            case EXTERN_OR_INTRIN:
                cppp::unreachable();
            case UINT32SYM:
            case NTYPE:
                throw std::logic_error("DataFlowGraph::compile(): Unexpected node type "s+std::to_string(std::to_underlying(nd.type())));
        }
        cppp::unreachable();
    }
    DataFlowGraph::DataFlowGraph(const ProjectEntitiesPool& pep,const bbe::Function& f) : _root(compile(pep,main,f.ast())){}
}
