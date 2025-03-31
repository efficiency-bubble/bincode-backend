#include"assembly.hpp"
namespace bbe::impl{
    static Value compile_node(const ASTNode& nd,FunctionCompilationContext& fcc){
        switch(nd.type()){
            case 0:{ // ret
                Value value{compile_node(nd.ugetc(0),fcc)};
                x86::encode::mov::r_rm<x86::width::W32>(fcc.text(),x86::reg::A,x86::DisplacementRM<x86::width::W8>(x86::reg::BP),value.frame_offset);
                x86::encode::ret::near(fcc.text());
                return {0};
            }
            case 1:{ // sub
                Value lhv{compile_node(nd.ugetc(0),fcc)};
                Value rhv{compile_node(nd.ugetc(1),fcc)};
                x86::encode::mov::r_rm<x86::width::W32>(fcc.text(),x86::reg::A,x86::DisplacementRM<x86::width::W8>(x86::reg::BP),rhv.frame_offset);
                fcc.stack().pop(rhv.frame_offset>>2);
                x86::encode::sub::rm_r<x86::width::W32>(fcc.text(),x86::DisplacementRM<x86::width::W8>(x86::reg::BP),lhv.frame_offset,x86::reg::A);
                return lhv;
            }
            case 2:{ // lit
                using asm_generic::operator""_b;
                std::uint32_t constant = nd.getp(0);
                std::uint64_t frame_offset = fcc.stack().push() << 2;
                std::uint8_t disp8 = std::saturate_cast<std::uint8_t>(frame_offset);
                if(disp8 == frame_offset){
                    x86::encode::mov::rm_imm<x86::width::W32>(fcc.text(),x86::DisplacementRM<x86::width::W8>(x86::reg::BP),constant,disp8);
                }else{
                    x86::encode::mov::rm_imm<x86::width::W32>(fcc.text(),x86::DisplacementRM<x86::width::W32>(x86::reg::BP),constant,frame_offset);
                }
                return {.frame_offset = frame_offset};
            }
            default:
                throw 3;
        }
    }
    void Default_AMD64::compile(const Function& fn,Text& t) const{
        FunctionCompilationContext fcc;
        compile_node(fn.ast(),fcc);
        x86::encode::sub::rm_imm<x86::width::W32>(t.text(),x86::RM(x86::reg::BP),4*fcc.stack().max_size());
        t.text().append(fcc.text());
    }
}
