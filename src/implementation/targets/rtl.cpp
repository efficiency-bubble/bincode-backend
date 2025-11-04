#include<bbe/targets/rtl.hpp>
#include<stdexcept>
namespace bbe::targets::rtl::impl{
    template<typename T>
    std::uint32_t next(std::vector<T>& vec){
        std::uint32_t r = static_cast<std::uint32_t>(vec.size());
        vec.emplace_back();
        return r;
    }
    constexpr static std::uint32_t nval = std::numeric_limits<std::uint32_t>::max();
    class FunctionCompiler{
        Function& fn;
        std::uint32_t reg;
        std::unordered_map<const dfg::DataNode*,std::uint32_t> cache;
        std::vector<std::uint32_t> magic_results_stash;
        std::vector<std::vector<std::uint32_t>> packs;
        std::uint32_t next_reg(){
            return reg++;
        }
        std::uint32_t _compile_node(const dfg::DataNode* node){
            switch(node->operation()){
                case 0: case 20: { // u32 / bool
                    std::uint32_t r = next_reg();
                    fn.ins.emplace_back(Operation::LDI,r,node->primitive());
                    return r;
                }
                case 2: { // pack
                    std::uint32_t pid = next(packs);
                    for(const dfg::DataNode* p : node->parents()){
                        packs[pid].emplace_back(compile_node(p));
                    }
                    return pid;
                }
                case 9: { // cmag
                    const auto& par = node->parents();
                    switch(node->primitive()){
                        case 25:
                            static_cast<void>(compile_node(par.back())); // place side effect before
                            fn.ins.emplace_back(Operation::PRI,compile_node(par.front()));
                            return nval;
                        default:
                            throw std::logic_error("RTL compile: unknown magic "s+std::to_string(node->primitive()));
                    }
                }
                case 21: { // fork
                    const auto& par = node->parents();
                    std::uint32_t result = nval;
                    std::uint32_t cond = compile_node(par.front());
                    std::uint32_t elselid = next(fn.labels);
                    std::uint32_t donelid = next(fn.labels);
                    fn.ins.emplace_back(Operation::JZ,elselid,cond);
                    if(std::uint32_t tr = compile_node(par[1uz]);tr != nval){
                        result = next_reg();
                        fn.ins.emplace_back(Operation::MOV,result,tr);
                    }
                    fn.labels[elselid] = fn.ins.size();
                    fn.ins.emplace_back(Operation::JMP,donelid,0);
                    if(std::uint32_t fr = compile_node(par[2uz]);fr != nval){
                        if(result == nval) result = next_reg();
                        fn.ins.emplace_back(Operation::MOV,result,fr);
                    }
                    fn.labels[donelid] = fn.ins.size();
                    return result;
                }
                case 350: { // sequence point
                    for(const dfg::DataNode* par : node->parents()){
                        static_cast<void>(compile_node(par));
                    }
                    return nval;
                }
                case std::numeric_limits<std::uint32_t>::max():
                    return nval;
                default:
                    throw std::logic_error("RTL compile: unknown node type "s+std::to_string(node->operation()));
            }
        }
        public:
            FunctionCompiler(Function& f) : fn(f), reg(0){}
            std::uint32_t compile_node(const dfg::DataNode* node){
                if(auto it=cache.find(node);it!=cache.end()){
                    return it->second;
                }else{
                    return cache.try_emplace(node,_compile_node(node)).first->second;
                }
            }
    };
    Function::Function(const dfg::DataFlowGraph& fg){
        FunctionCompiler compiler{*this};
        for(const dfg::DataNode* v : fg.envp()){
            compiler.compile_node(v);
        }
        if(std::uint32_t rv = compiler.compile_node(fg.root());rv != nval){
            ins.emplace_back(Operation::RET,rv,0);
        }
    }
}
