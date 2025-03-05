#pragma once
#include"commons.hpp"
#include"assembly.hpp"
#include<assembly/instruction.hpp>
#include<cppp/optional.hpp>
#include<cstdint>
#include<numeric>
#include<utility>
#include<vector>
#include<set>
#include<map>
namespace bbe::impl{
    struct Value{
        std::uint64_t frame_offset;
    };
    class ASTNode;
    class ASTNodeDefs;
    struct ASTNodeDef{
        std::uint64_t nchld;
        Value(*compilef)(const ASTNodeDefs&,ASTNode&,FunctionCompilationContext&);
    };
    class ASTNodeDefs{
        std::map<std::uint64_t,ASTNodeDef> defs;
        public:
            const ASTNodeDef& get(std::uint64_t i) const{
                return defs.at(i);
            }
            template<typename ...A>
            void emplace(std::uint64_t i,A&& ...a){
                defs.try_emplace(i,std::forward<A>(a)...);
            }
            inline Value compile(ASTNode& nd,FunctionCompilationContext& fcc) const;
    };
    struct data_tag_t{};
    constexpr inline data_tag_t data_tag{};
    class ASTNode{
        std::uint64_t _type;
        using chld_t = std::vector<cppp::optional<ASTNode>>;
        std::variant<
            chld_t,
            std::vector<std::uint32_t>
        > data;
        public:
            std::uint64_t type() const{
                return _type;
            }
            const ASTNode& ugetc(std::uint64_t ind) const{
                return *std::get<chld_t>(data)[ind];
            }
            ASTNode& ugetc(std::uint64_t ind){
                return *std::get<chld_t>(data)[ind];
            }
            const ASTNode* getc(std::uint64_t ind) const{
                return std::get<chld_t>(data)[ind].ptr();
            }
            ASTNode* getc(std::uint64_t ind){
                return std::get<chld_t>(data)[ind].ptr();
            }
            std::uint32_t getp(std::uint64_t ind) const{
                return std::get<std::vector<std::uint32_t>>(data)[ind];
            }
            void setp(std::uint64_t ind,std::uint32_t p){
                std::get<std::vector<std::uint32_t>>(data)[ind] = p;
            }
            template<typename ...A>
            ASTNode& emplace(std::uint64_t ind,A&& ...a){
                std::get<chld_t>(data)[ind].emplace(std::forward<A>(a)...);
                return *std::get<chld_t>(data)[ind];
            }
            ASTNode(std::uint64_t tp,std::uint64_t nchld) : _type(tp), data(std::in_place_type<chld_t>,nchld){}
            ASTNode(std::uint64_t tp,std::uint64_t nchld,data_tag_t) : _type(tp), data(std::in_place_type<std::vector<std::uint32_t>>,nchld){}
    };
    inline Value ASTNodeDefs::compile(ASTNode& nd,FunctionCompilationContext& fcc) const{
        return get(nd.type()).compilef(*this,nd,fcc);
    }
    ASTNodeDefs default_ast_defs(){
        ASTNodeDefs defs;
        defs.emplace(0,2,[](const ASTNodeDefs& d,ASTNode& nd,FunctionCompilationContext& context) -> Value {
            Value value{d.compile(nd.ugetc(0),context)};
            x86::encode::mov::r_rm<x86::width::W32>(context.text().text(),x86::reg::A,x86::encode::DisplacementRM<x86::width::W8>(x86::reg::BP),value.frame_offset);
            x86::encode::ret::near(context.text().text());
            return {0};
        });
        defs.emplace(1,2,[](const ASTNodeDefs& d,ASTNode& nd,FunctionCompilationContext& context) -> Value {
            Value lhv{d.compile(nd.ugetc(0),context)};
            Value rhv{d.compile(nd.ugetc(1),context)};
            x86::encode::mov::r_rm<x86::width::W32>(context.text().text(),x86::reg::A,x86::encode::DisplacementRM<x86::width::W8>(x86::reg::BP),rhv.frame_offset);
            context.stack().pop(rhv.frame_offset>>2);
            x86::encode::sub::rm_r<x86::width::W32>(context.text().text(),x86::encode::DisplacementRM<x86::width::W8>(x86::reg::BP),lhv.frame_offset,x86::reg::A);
            return lhv;
        });
        defs.emplace(2,2,[](const ASTNodeDefs&,ASTNode& nd,FunctionCompilationContext& context) -> Value {
            using asm_generic::operator""_b;
            std::uint32_t constant = nd.getp(0);
            std::uint64_t frame_offset = context.stack().push() << 2;
            std::uint8_t disp8 = std::saturate_cast<std::uint8_t>(frame_offset);
            if(disp8 == frame_offset){
                x86::encode::mov::rm_imm<x86::width::W32>(context.text().text(),x86::encode::DisplacementRM<x86::width::W8>(x86::reg::BP),constant,disp8);
            }else{
                x86::encode::mov::rm_imm<x86::width::W32>(context.text().text(),x86::encode::DisplacementRM<x86::width::W32>(x86::reg::BP),constant,frame_offset);
            }
            return {.frame_offset = frame_offset};
        });
        return defs;
    };
}
namespace bbe{
    BBE_EXPORT Value;
    BBE_EXPORT ASTNode;
    BBE_EXPORT data_tag;
    BBE_EXPORT ASTNodeDef;
    BBE_EXPORT ASTNodeDefs;
    BBE_EXPORT default_ast_defs;
}
