#include<bbe/targets/ssa.hpp>
namespace bbe::targets::ssa::impl{
    std::uint32_t ProcedureIC::new_label_here(){
        std::uint32_t lid = labels.size();
        Label& l = labels.emplace_back();
        for(std::uint32_t n=0;n<static_cast<std::uint32_t>(name_values.size());++n){
            l.param_names.emplace_back(n);
            _instructions.emplace_back(Operation::PUTV,n,name_values[n]);
        }
        l.begin = _instructions.size();
        for(std::uint32_t n=0;n<static_cast<std::uint32_t>(name_values.size());++n){
            operation(Operation::LOADV,n,n);
        }
        return lid;
    }
    std::uint32_t compile_node(const ASTNode& nd,ProcedureIC& prog){
        // new_name() doesn't allocate a name unless it's actually used, so this is always safe.
        std::uint32_t name{prog.new_name()};
        switch(nd.type()){
            case 0: // u32
                prog.imm32(name,nd.getp(0));
                break;
            case 1: // u64
                prog.imm64(name,nd.getp(0));
                break;
            case 2: // add
                prog.operation(Operation::ADD,name,prog.value_of(compile_node(nd.getc(0),prog)),prog.value_of(compile_node(nd.getc(1),prog)));
                break;
            case 3: // sub
                prog.operation(Operation::SUB,name,prog.value_of(compile_node(nd.getc(0),prog)),prog.value_of(compile_node(nd.getc(1),prog)));
                break;
            case 5: // arg32
            case 6: // arg64
                prog.operation(Operation::LDAR,name,nd.getp(0));
                break;
            case 7: // ret
                prog.statement(Operation::RET,prog.value_of(compile_node(nd.getc(0),prog)));
                break;
            // case 8: // callf // TODO
            //     break;
            // case 100: // sym32 // TODO
            // case 101: // sym64
            //     return fcc.symbol(nd.getp(0),x86::width::W64);
            default:
                throw 3;
        }
        return name;
    }
    void ProcedureIC::compile(const Function& fn){
        compile_node(fn.ast(),*this);
    }
}
