#pragma once
#include<bbe/bbe.hpp>
#include<cppp/string.hpp>
using namespace bbe;
using namespace std::literals;
using namespace cppp::literals;

constexpr static std::uint32_t INTR_ADDU32 = 10;
constexpr static std::uint32_t INTR_ADDS32 = 11;
constexpr static std::uint32_t INTR_SUBU32 = 20;
constexpr static std::uint32_t INTR_SUBS32 = 21;
constexpr static std::uint32_t INTR_MULU32 = 30;
constexpr static std::uint32_t INTR_MULS32 = 31;
constexpr static std::uint32_t INTR_PRU32 = 100;
constexpr static std::uint32_t INTR_EQU32 = 50;
constexpr static std::uint32_t INTR_LEQU32 = 51;
constexpr static std::uint32_t INTR_BNOT = 60;

ASTNode u32(std::uint32_t val){
    return {NodeType::UINT32,val};
}
ASTNode cbool(bool val){
    return {NodeType::BOOL,val};
}
template<std::same_as<ASTNode> ...T>
ASTNode pack(T&& ...children){
    ASTNode x{NodeType::PACK,sizeof...(T),uninitialize};
    template for(constexpr std::uint32_t i : std::views::indices(cppp::safe_cast<std::uint32_t>(sizeof...(T)))){
        x.children()[i].initialize(std::forward<T...[i]>(children...[i]));
    }
    return x;
}
template<typename ...T>
ASTNode comma(std::uint32_t ind,T&& ...children){
    ASTNode x{NodeType::COMMA,ind,sizeof...(T),uninitialize};
    template for(constexpr std::uint32_t i : std::views::indices(cppp::safe_cast<std::uint32_t>(sizeof...(T)))){
        x.children()[i].initialize(std::forward<T...[i]>(children...[i]));
    }
    return x;
}
ASTNode fn(std::uint32_t id){
    return {NodeType::FNSYM,id};
}
ASTNode pind(ASTNode&& arg,std::uint32_t ind){
    ASTNode x{NodeType::PACKIND,ind,1,uninitialize};
    x.children()[0_u32].initialize(std::move(arg));
    return x;
}
ASTNode addrof(ASTNode&& arg){
    ASTNode x{NodeType::ADDROF,1,uninitialize};
    x.children()[0_u32].initialize(std::move(arg));
    return x;
}
ASTNode deref(ASTNode&& arg){
    ASTNode x{NodeType::DEREF,1,uninitialize};
    x.children()[0_u32].initialize(std::move(arg));
    return x;
}
ASTNode arg(std::uint32_t ind){
    return {NodeType::ARG,ind};
}
template<std::same_as<ASTNode> ...T>
ASTNode call(T&& ...children){
    ASTNode x{NodeType::CALL,sizeof...(T),uninitialize};
    template for(constexpr std::uint32_t i : std::views::indices(cppp::safe_cast<std::uint32_t>(sizeof...(T)))){
        x.children()[i].initialize(std::forward<T...[i]>(children...[i]));
    }
    return x;
}
template<std::same_as<ASTNode> ...T>
ASTNode call(const bbe::Function& f,T&& ...children){
    return call(fn(f.index()),std::forward<T>(children)...);
}
ASTNode fork(ASTNode&& cond,ASTNode&& tru,ASTNode&& fals){
    ASTNode x{NodeType::FORK,3,uninitialize};
    x.children()[0_u32].initialize(std::move(cond));
    x.children()[1_u32].initialize(std::move(tru));
    x.children()[2_u32].initialize(std::move(fals));
    return x;
}
ASTNode setvar(std::uint32_t var,ASTNode&& val){
    ASTNode x{NodeType::SETVAR,var,1,uninitialize};
    x.children()[0_u32].initialize(std::move(val));
    return x;
}
ASTNode havevar(std::uint32_t var,ASTNode&& val,ASTNode&& expr){
    ASTNode x{NodeType::HAVEVAR,var,2,uninitialize};
    x.children()[0_u32].initialize(std::move(val));
    x.children()[1_u32].initialize(std::move(expr));
    return x;
}
ASTNode getvar(std::uint32_t var){
    return {NodeType::GETVAR,var};
}
FunctionSignature intrin_sig(std::uint32_t i,const TypeDatabase& tdb){
    switch(i){
        case 10: case 20: case 30: case 50: case 51:
            return {tdb[T_UINT32],{tdb[T_UINT32],tdb[T_UINT32]}};
        case 11: case 21: case 31:
            return {tdb[T_INT32],{tdb[T_INT32],tdb[T_INT32]}};
        case 60:
            return {tdb[T_BOOL],{tdb[T_BOOL]}};
        case 100:
            return {tdb[T_VOID],{tdb[T_UINT32]}};
    }
    cppp::unreachable();
}
const Function& intrin(std::uint32_t i,ProjectEntitiesPool& pep){
    return pep.functions().emplace(i,intrin_sig(i,pep.types()));
}
