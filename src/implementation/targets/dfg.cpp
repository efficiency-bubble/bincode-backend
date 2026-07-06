#include<bbe/targets/dfg.hpp>
#include<unordered_set>
#include<stdexcept>
#include<cassert>
#include<ranges>
#include<string>
namespace bbe::targets::dfg::impl{
    using namespace std::literals;
    const DataNode* DataFlowGraph::compile(CodeBranch& br,const ASTNode& nd){
        switch(nd.type()){
            using enum bbe::NodeType;
            case UINT32:
                return &_nodes.emplace_back(NodeType::UINT32,nd.result_type(),nd.getp32());
            case SINT32:
                return &_nodes.emplace_back(NodeType::SINT32,nd.result_type(),nd.getp32());
            case UINT64:
                throw std::logic_error("dfg::compile(): Unsupported node type uint64"s);
            case PACK: {
                //TODO: customizable ordering (currently only sequential)
                DataNode* pack = &_nodes.emplace_back(NodeType::PACK,nd.result_type());
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
                return &_nodes.emplace_back(NodeType::PACKIND,nd.result_type(),nd.getp32(),std::vector{compile(br,nd.children().front())});
            case ARG:
                return &_nodes.emplace_back(NodeType::ARG,nd.result_type());
            case CALL_BUILTIN: {
                std::uint32_t fnid = nd.getp32();
                switch(fnid){
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
                    default:;
                }
                DataNode& cmag = _nodes.emplace_back(NodeType::CALL_BUILTIN,nd.result_type(),fnid);
                for(const ASTNode& c : nd.children()){
                    cmag.emplace(compile(br,c));
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
                return &_nodes.emplace_back(NodeType::BOOL,nd.result_type(),nd.getp32());
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
                    DataNode* fork = &_nodes.emplace_back(NodeType::FORK,br.getvar(v)->return_type());
                    fork->emplace(condition);
                    fork->emplace(lcb.getvar(v));
                    fork->emplace(rcb.getvar(v));
                    br.setvar(v,fork);
                }
                DataNode* join = &_nodes.emplace_back(NodeType::FORK,nd.result_type());
                join->emplace(condition);
                join->emplace(lhs);
                join->emplace(rhs);
                return join;
            }
            case FNSYM:
                return &_nodes.emplace_back(NodeType::FNSYM,nd.result_type(),nd.getp32());
            default:
                throw std::logic_error("DataFlowGraph::compile(): Unknown node type "s+std::to_string(std::to_underlying(nd.type())));
        }
    }
    DataFlowGraph::DataFlowGraph(const bbe::Function& f) : _root((main.setvar(IO_VAR,&_nodes.emplace_back(NodeType::STDOUT,TypeDatabase::T_VOID)),compile(main,f.ast()))){}
}
