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
    class FunctionCompiler{
        Function& fn;
        std::unordered_map<const dfg::DataNode*,std::uint32_t> cache;
        std::uint32_t next_reg(){
            return fn.nvals++;
        }
        std::uint32_t compile_fork(const dfg::DataNode* cond,const dfg::DataNode* tru,const dfg::DataNode* fals){
            std::uint32_t result = NVAL;
            fork(cond,[this,tru,&result]{
                if(std::uint32_t tr = compile_node(tru);tr != NVAL){
                    result = next_reg();
                    fn.ins.emplace_back(Operation::MOV,result,tr);
                }
            },[this,fals,&result]{
                if(std::uint32_t fr = compile_node(fals);fr != NVAL){
                    if(result == NVAL) result = next_reg();
                    fn.ins.emplace_back(Operation::MOV,result,fr);
                }
            });
            return result;
        }
        std::uint32_t _compile_node(const dfg::DataNode* node){
            switch(node->operation()){
                case 0: case 20: { // u32 / bool
                    std::uint32_t r = next_reg();
                    fn.ins.emplace_back(Operation::LDI,r,node->primitive());
                    return r;
                }
                case 2: { // pack
                    std::uint32_t r = next_reg();
                    fn.ins.emplace_back(Operation::MKPACK,r);
                    for(const dfg::DataNode* p : node->parents()){
                        fn.ins.emplace_back(Operation::PACKATT,r,compile_node(p));
                    }
                    return r;
                }
                case 4: { // packind
                    std::uint32_t r = next_reg();
                    fn.ins.emplace_back(Operation::MOV,r,_compile_node(node->parents().front()));
                    fn.ins.emplace_back(Operation::IPACK,r,node->primitive());
                    return r;
                }
                case 5: { // argv
                    std::uint32_t r = next_reg();
                    fn.ins.emplace_back(Operation::ARGV,r);
                    return r;
                }
                case 9: { // cmag
                    const auto& par = node->parents();
                    switch(node->primitive()){
                        case 0: {
                            std::uint32_t r = next_reg();
                            fn.ins.emplace_back(Operation::MOV,r,compile_node(par[1uz]));
                            fn.ins.emplace_back(Operation::CALL,r,compile_node(par[0uz]));
                            return r;
                        }
                        case 10: {
                            std::uint32_t r = next_reg();
                            fn.ins.emplace_back(Operation::MOV,r,compile_node(par[0uz]));
                            fn.ins.emplace_back(Operation::ADD,r,compile_node(par[1uz]));
                            return r;
                        }
                        case 11: {
                            std::uint32_t r = next_reg();
                            fn.ins.emplace_back(Operation::MOV,r,compile_node(par[0uz]));
                            fn.ins.emplace_back(Operation::SUB,r,compile_node(par[1uz]));
                            return r;
                        }
                        case 25:
                            static_cast<void>(compile_node(par[1uz])); // place side effect before
                            fn.ins.emplace_back(Operation::PRI,compile_node(par[0uz]));
                            return NVAL;
                        case 51: {
                            std::uint32_t r = next_reg();
                            fn.ins.emplace_back(Operation::MOV,r,compile_node(par[0uz]));
                            fn.ins.emplace_back(Operation::CLE,r,compile_node(par[1uz]));
                            return r;
                        }
                        default:
                            throw std::logic_error("RTL compile: unknown magic "s+std::to_string(node->primitive()));
                    }
                }
                case 21: { // fork
                    const auto& par = node->parents();
                    return compile_fork(par[0uz],par[1uz],par[2uz]);
                }
                case 200: { // fptr
                    std::uint32_t r = next_reg();
                    fn.ins.emplace_back(Operation::LDFN,r,node->primitive());
                    return r;
                }
                case 400: // _stdout
                case std::numeric_limits<std::uint32_t>::max():
                    return NVAL;
                default:
                    throw std::logic_error("RTL compile: unknown node type "s+std::to_string(node->operation()));
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
                std::uint32_t elselid = next(fn.labels);
                std::uint32_t donelid = next(fn.labels);
                fn.ins.emplace_back(Operation::JF,elselid,condv);
                tru();
                fn.ins.emplace_back(Operation::JMP,donelid,0);
                fn.labels[elselid] = std::prev(fn.ins.end());
                fals();
                fn.labels[donelid] = std::prev(fn.ins.end());
            }
    };
    Function::Function(const dfg::DataFlowGraph& fg) : nvals(0){
        FunctionCompiler compiler{*this};
        std::uint32_t rv = compiler.compile_node(fg.root());
        compiler.compile_node(fg.stdout_result());
        if(rv != NVAL){
            ins.emplace_back(Operation::RET,rv,0);
        }
    }
}
