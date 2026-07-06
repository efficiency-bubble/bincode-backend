#include<bbe/targets/rtl.hpp>
#include<stdexcept>
namespace bbe::targets::rtl::impl{
    template<typename T>
    static std::uint32_t next(std::vector<T>& vec){
        std::uint32_t r = static_cast<std::uint32_t>(vec.size());
        vec.emplace_back();
        return r;
    }
    constexpr static std::uint32_t NVAL = std::numeric_limits<std::uint32_t>::max();
    namespace{
        class FunctionCompiler{
            Function& fn;
            std::unordered_map<const dfg::DataNode*,std::uint32_t> cache;
            std::uint32_t compile_fork(const dfg::DataNode* cond,const dfg::DataNode* tru,const dfg::DataNode* fals){
                std::uint32_t result = NVAL;
                fork(cond,[this,tru,&result]{
                    if(std::uint32_t tr = compile_node(tru);tr != NVAL){
                        result = fn.alloc_val();
                        fn.add_instruction(Operation::MOV,result,tr);
                    }
                },[this,fals,&result]{
                    if(std::uint32_t fr = compile_node(fals);fr != NVAL){
                        if(result == NVAL) result = fn.alloc_val();
                        fn.add_instruction(Operation::MOV,result,fr);
                    }
                });
                return result;
            }
            std::uint32_t _compile_node(const dfg::DataNode* node){
                switch(node->operation()){
                    using enum dfg::NodeType;
                    case UINT32: case BOOL: {
                        std::uint32_t r = fn.alloc_val();
                        fn.add_instruction(Operation::LDI,r,node->primitive());
                        return r;
                    }
                    case PACK: {
                        std::uint32_t r = fn.alloc_val();
                        fn.add_instruction(Operation::MKPACK,r);
                        for(const dfg::DataNode* p : node->parents()){
                            fn.add_instruction(Operation::PACKATT,r,compile_node(p));
                        }
                        return r;
                    }
                    case PACKIND: {
                        std::uint32_t r = fn.alloc_val();
                        fn.add_instruction(Operation::MOV,r,_compile_node(node->parents().front()));
                        fn.add_instruction(Operation::IPACK,r,node->primitive());
                        return r;
                    }
                    case ARG: {
                        std::uint32_t r = fn.alloc_val();
                        fn.add_instruction(Operation::ARG,r);
                        return r;
                    }
                    case CALL_BUILTIN: {
                        const auto& par = node->parents();
                        switch(node->primitive()){
                            case 0: {
                                std::uint32_t r = fn.alloc_val();
                                fn.add_instruction(Operation::MOV,r,compile_node(par[1uz]));
                                fn.add_instruction(Operation::CALL,r,compile_node(par[0uz]));
                                return r;
                            }
                            case 10: {
                                std::uint32_t r = fn.alloc_val();
                                fn.add_instruction(Operation::MOV,r,compile_node(par[0uz]));
                                fn.add_instruction(Operation::ADD,r,compile_node(par[1uz]));
                                return r;
                            }
                            case 20: {
                                std::uint32_t r = fn.alloc_val();
                                fn.add_instruction(Operation::MOV,r,compile_node(par[0uz]));
                                fn.add_instruction(Operation::SUB,r,compile_node(par[1uz]));
                                return r;
                            }
                            case 50: {
                                std::uint32_t r = fn.alloc_val();
                                fn.add_instruction(Operation::MOV,r,compile_node(par[0uz]));
                                fn.add_instruction(Operation::CEQ,r,compile_node(par[1uz]));
                                return r;
                            }
                            case 51: {
                                std::uint32_t r = fn.alloc_val();
                                fn.add_instruction(Operation::MOV,r,compile_node(par[0uz]));
                                fn.add_instruction(Operation::CLE,r,compile_node(par[1uz]));
                                return r;
                            }
                            default:
                                throw std::logic_error("RTL compile: unknown magic "s+std::to_string(node->primitive()));
                        }
                    }
                    case FORK: {
                        const auto& par = node->parents();
                        return compile_fork(par[0uz],par[1uz],par[2uz]);
                    }
                    case FNSYM: {
                        std::uint32_t r = fn.alloc_val();
                        fn.add_instruction(Operation::LDFN,r,node->primitive());
                        return r;
                    }
                    case STDOUT: // _stdout
                        return NVAL;
                    default:
                        throw std::logic_error("RTL compile: unknown node type "s+std::to_string(std::to_underlying(node->operation())));
                }
            }
            public:
                FunctionCompiler(Function& f) : fn(f){}
                std::uint32_t compile_node(const dfg::DataNode* node){
                    if(auto it=cache.find(node);it!=cache.end()){
                        return it->second;
                    }else{
                        return cache.try_emplace(node,_compile_node(node)).first->second;
                    }
                }
                template<typename Ft,typename Ff>
                void fork(const dfg::DataNode* cond,Ft&& tru,Ff&& fals){
                    std::uint32_t condv = compile_node(cond);
                    std::uint32_t elselid = next(fn.labels());
                    std::uint32_t donelid = next(fn.labels());
                    fn.add_instruction(Operation::JF,elselid,condv);
                    tru();
                    fn.add_instruction(Operation::JMP,donelid,0);
                    fn.labels()[elselid] = std::prev(fn.instructions().end());
                    fals();
                    fn.labels()[donelid] = std::prev(fn.instructions().end());
                }
        };
    }
    Function::Function(const dfg::DataFlowGraph& fg) : nvals(0){
        FunctionCompiler compiler{*this};
        std::uint32_t rv = compiler.compile_node(fg.root());
        compiler.compile_node(fg.stdout_result());
        ins.emplace_back(Operation::RET,rv,0);
    }
}
