#pragma once
#include"commons.hpp"
#include"assembly.hpp"
#include<assembly/instruction.hpp>
#include<cppp/virtual.hpp>
#include<cstdint>
#include<numeric>
#include<utility>
#include<vector>
namespace bbe::impl{
    struct Value{
        std::uint64_t frame_offset;
    };
    class ASTNode : public cppp::virtual_class{
        public:
            virtual void set(std::uint64_t,ptr<ASTNode>&&){}
            virtual void setprim32(std::uint64_t,std::uint32_t){
                throw std::runtime_error("Bad setprim32");
            }
            virtual Value compile(FunctionCompilationContext&) const = 0;
    };
    class Constanti32 : public ASTNode{
        std::uint32_t constant;
        public:
            Constanti32(std::uint32_t c) : constant(c){}
            virtual void setprim32(std::uint64_t,std::uint32_t u32){
                constant = u32;
            }
            Value compile(FunctionCompilationContext& context) const override{
                using asm_generic::operator""_b;
                std::uint64_t frame_offset = context.stack().push() << 2;
                std::uint8_t disp8 = std::saturate_cast<std::uint8_t>(frame_offset);
                if(disp8 == frame_offset){
                    x86::encode::mov::rm_imm<x86::width::W32>(context.text().text(),x86::encode::DisplacementRM<x86::width::W8>(x86::reg::BP),constant,disp8);
                }else{
                    x86::encode::mov::rm_imm<x86::width::W32>(context.text().text(),x86::encode::DisplacementRM<x86::width::W32>(x86::reg::BP),constant,frame_offset);
                }
                return {.frame_offset = frame_offset};
            }
    };
    class Subi32 : public ASTNode{
        ptr<ASTNode> lhs;
        ptr<ASTNode> rhs;
        public:
            Subi32(ptr<ASTNode>&& lhs,ptr<ASTNode>&& rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)){}
            void set(std::uint64_t ind,ptr<ASTNode>&& nd) override{
                if(ind){
                    rhs = std::move(nd);
                }else{
                    lhs = std::move(nd);
                }
            }
            Value compile(FunctionCompilationContext& context) const override{
                if(!lhs){
                    throw std::logic_error("Blank lhs to subi32");
                }
                if(!rhs){
                    throw std::logic_error("Blank rhs to subi32");
                }
                
                Value lhv{lhs->compile(context)};
                Value rhv{rhs->compile(context)};
                x86::encode::mov::r_rm<x86::width::W32>(context.text().text(),x86::reg::A,x86::encode::DisplacementRM<x86::width::W8>(x86::reg::BP),rhv.frame_offset);
                context.stack().pop(rhv.frame_offset>>2);
                x86::encode::sub::rm_r<x86::width::W32>(context.text().text(),x86::encode::DisplacementRM<x86::width::W8>(x86::reg::BP),lhv.frame_offset,x86::reg::A);
                return lhv;
            }
    };
    class Return : public ASTNode{
        ptr<ASTNode> en;
        public:
            Return(ptr<ASTNode>&& en) : en(std::move(en)){}
            void set(std::uint64_t,ptr<ASTNode>&& nd) override{
                en = std::move(nd);
            }
            Value compile(FunctionCompilationContext& context) const{
                if(!en){
                    throw std::logic_error("Blank return");
                }
                Value value{en->compile(context)};
                x86::encode::mov::r_rm<x86::width::W32>(context.text().text(),x86::reg::A,x86::encode::DisplacementRM<x86::width::W8>(x86::reg::BP),value.frame_offset);
                x86::encode::ret::near(context.text().text());
                return {0};
            }
    };
}
namespace bbe{
    BBE_EXPORT Value;
    BBE_EXPORT ASTNode;
    BBE_EXPORT Constanti32;
    BBE_EXPORT Subi32;
    BBE_EXPORT Return;
}
