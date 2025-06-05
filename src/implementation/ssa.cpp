#include<bbe/targets/ssa.hpp>
#include<unordered_map>
#include<functional>
#include<compare>
#include<ranges>
namespace bbe::targets::ssa::impl{
    std::uint32_t BasicBlock::new_value_for_name(std::uint32_t name){
        std::uint32_t vid = next_value++;
        name_values.insert_or_assign(name,vid);
        return vid;
    }
    class FunctionCompiler{
        ProcedureIC* ic;
        std::uint32_t name_count = 0;
        std::uint32_t new_name(){
            return name_count++;
        }
        std::uint32_t compile_value(const ASTNode& nd,BasicBlock& block){
            return block.value_of(compile_node(nd,block));
        }
        std::uint32_t compile_node(const ASTNode& nd,BasicBlock& block){
            std::uint32_t name;
            switch(nd.type()){
                case 0: // u32
                    block.imm32(name = new_name(),nd.getp(0));
                    break;
                case 1: // u64
                    block.imm64(name = new_name(),nd.getp(0));
                    break;
                case 2: // add
                    block.operation(Operation::ADD,name = new_name(),{compile_value(nd.getc(0),block),compile_value(nd.getc(1),block)});
                    break;
                case 3: // sub
                    block.operation(Operation::SUB,name = new_name(),{compile_value(nd.getc(0),block),compile_value(nd.getc(1),block)});
                    break;
                case 5: // arg32
                case 6: // arg64
                case 22: // argb
                    block.operation(Operation::LDAR,name = new_name(),{static_cast<std::uint32_t>(nd.getp(0))});
                    break;
                case 7: // ret
                    block.retf(compile_value(nd.getc(0),block));
                    return BasicBlock::NNAME;
                // case 8: // callf // TODO
                //     break;
                case 21:{ // fork
                    std::uint32_t cond = compile_value(nd.getc(0),block);
                    std::uint32_t lhb = compile_block(nd.getc(1));
                    std::uint32_t rhb = compile_block(nd.getc(2));
                    block.operation(Operation::BRC,name = new_name(),{cond,lhb,rhb});
                    break;
                }
                // case 100: // sym32 // TODO
                // case 101: // sym64
                //     return fcc.symbol(nd.getp(0),x86::width::W64);
                default:
                    throw 3;
            }
            return name;
        }
        public:
            FunctionCompiler(ProcedureIC& c) : ic(&c){}
            std::uint32_t compile_block(const ASTNode& nd){
                std::uint32_t bid = ic->new_block();
                ic->blocks()[bid].retb(compile_value(nd,ic->blocks()[bid]));
                return bid;
            }
    };
    void ProcedureIC::compile(const Function& fn){
        FunctionCompiler fc{*this};
        fc.compile_block(fn.ast());
    }
}
