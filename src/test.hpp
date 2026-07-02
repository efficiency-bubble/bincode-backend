#pragma once
#include<bbe/bbe.hpp>
#include<cppp/string.hpp>
using namespace bbe;

constexpr static std::uint32_t FN_CALL = 0;
constexpr static std::uint32_t FN_ADD32 = 10;
constexpr static std::uint32_t FN_SUB32 = 20;
constexpr static std::uint32_t FN_MUL32 = 30;
constexpr static std::uint32_t FN_PRU32 = 25;
constexpr static std::uint32_t FN_EQ32 = 50;
constexpr static std::uint32_t FN_LEQ32 = 51;
constexpr static std::uint32_t FN_BNOT = 60;

ASTNode u32(std::uint32_t val){
    return {NodeType::UINT32,val};
}
ASTNode cbool(bool val){
    return {NodeType::BOOL,val};
}
template<typename ...T>
ASTNode pack(T&& ...children){
    ASTNode x{NodeType::PACK,sizeof...(T),uninitialize};
    template for(constexpr std::size_t i : std::views::indices(sizeof...(T))){
        x.children()[i].initialize(std::forward<T...[i]>(children...[i]));
    }
    return x;
}
template<typename ...T>
ASTNode comma(std::uint32_t ind,T&& ...children){
    ASTNode x{NodeType::COMMA,ind,sizeof...(T),uninitialize};
    template for(constexpr std::size_t i : std::views::indices(sizeof...(T))){
        x.children()[i].initialize(std::forward<T...[i]>(children...[i]));
    }
    return x;
}
ASTNode fn(std::uint32_t id){
    return {NodeType::FNSYM,id};
}
ASTNode arg(){
    return {NodeType::ARG};
}
ASTNode pind(ASTNode&& arg,std::uint32_t ind){
    ASTNode x{NodeType::PACKIND,ind,1,uninitialize};
    x.children()[0uz].initialize(std::move(arg));
    return x;
}
ASTNode arg(std::uint32_t ind){
    return pind(arg(),ind);
}
template<typename ...T>
ASTNode cmag(std::uint32_t magic,T&& ...children){
    ASTNode x{NodeType::CALL_BUILTIN,magic,sizeof...(T),uninitialize};
    template for(constexpr std::size_t i : std::views::indices(sizeof...(T))){
        x.children()[i].initialize(std::forward<T...[i]>(children...[i]));
    }
    return x;
}
ASTNode fork(ASTNode&& cond,ASTNode&& tru,ASTNode&& fals){
    ASTNode x{NodeType::FORK,3,uninitialize};
    x.children()[0uz].initialize(std::move(cond));
    x.children()[1uz].initialize(std::move(tru));
    x.children()[2uz].initialize(std::move(fals));
    return x;
}
ASTNode setvar(std::uint32_t var,ASTNode&& val){
    ASTNode x{NodeType::SETVAR,var,1,uninitialize};
    x.children()[0uz].initialize(std::move(val));
    return x;
}
ASTNode getvar(std::uint32_t var){
    return {NodeType::GETVAR,var};
}
using namespace std::literals;
std::unordered_map<std::uint32_t,cppp::sv> EXPLAIN{
    {0,u8"uint32"sv},
    {2,u8"pack"sv},
    {5,u8"arg"sv},
    {9,u8"cmag"sv},
    {21,u8"fork"sv},
    {300,u8"proxy"sv},
    {301,u8"lctrl"sv}
};

ASTNode fibonacci(){
    return fork(
        cmag(FN_LEQ32,arg(),u32(2)),
        u32(1),
        cmag(FN_ADD32,
            cmag(0,fn(0),cmag(FN_SUB32,arg(),u32(1))),
            cmag(0,fn(0),cmag(FN_SUB32,arg(),u32(2)))
        )
    );
}
