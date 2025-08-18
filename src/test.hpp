#pragma once
#include<bbe/bbe.hpp>
using namespace bbe;
constexpr static std::uint64_t NT_U32 = 0;
constexpr static std::uint64_t NT_U64 = 1;
constexpr static std::uint64_t NT_PACK = 2;
constexpr static std::uint64_t NT_COMMA = 3;
constexpr static std::uint64_t NT_ARG32 = 5;
constexpr static std::uint64_t NT_ARG64 = 6;
constexpr static std::uint64_t NT_RET = 7;
constexpr static std::uint64_t NT_CALL = 8;
constexpr static std::uint64_t NT_CMAG = 9;
constexpr static std::uint64_t NT_SETVAR = 10;
constexpr static std::uint64_t NT_GETVAR = 11;
constexpr static std::uint64_t NT_BOOL = 20;
constexpr static std::uint64_t NT_FORK = 21;
constexpr static std::uint64_t NT_ARGB = 22;
constexpr static std::uint64_t NT_COMPOUND = 64;
constexpr static std::uint64_t NT_SYM32 = 100;
constexpr static std::uint64_t NT_SYM64 = 101;

constexpr static std::uint32_t FN_ADD = 10;
constexpr static std::uint32_t FN_PRU32 = 25;

ASTNode u32(std::uint32_t val){
    return {NT_U32,val,0};
}
ASTNode cbool(bool val){
    return {NT_BOOL,val,0};
}
ASTNode arg32(std::uint32_t id){
    return {NT_ARG32,id,0};
}
ASTNode argb(std::uint32_t id){
    return {NT_ARGB,id,0};
}
template<typename ...T>
ASTNode pack(T&& ...children){
    ASTNode x{NT_PACK,sizeof...(T)};
    [&]<std::size_t ...i>(std::index_sequence<i...>){
        (... , x.emplace(i,std::forward<T>(children)));
    }(std::index_sequence_for<T...>());
    return x;
}
template<typename ...T>
ASTNode comma(std::size_t ind,T&& ...children){
    ASTNode x{NT_COMMA,ind,sizeof...(T)};
    [&]<std::size_t ...i>(std::index_sequence<i...>){
        (... , x.emplace(i,std::forward<T>(children)));
    }(std::index_sequence_for<T...>());
    return x;
}
ASTNode call(ASTNode&& fn,ASTNode&& arg){
    ASTNode x{NT_CALL,2};
    x.emplace(0,std::move(fn));
    x.emplace(1,std::move(arg));
    return x;
}
ASTNode cmag(std::uint64_t magic,ASTNode&& arg){
    ASTNode x{NT_CMAG,magic,2};
    x.emplace(0,std::move(arg));
    return x;
}
ASTNode ret(ASTNode&& val){
    ASTNode x{NT_RET,1};
    x.emplace(0,std::move(val));
    return x;
}
ASTNode fork(ASTNode&& cond,ASTNode&& tru,ASTNode&& fals){
    ASTNode x{NT_FORK,3};
    x.emplace(0,std::move(cond));
    x.emplace(1,std::move(tru));
    x.emplace(2,std::move(fals));
    return x;
}
ASTNode setvar(std::uint64_t var,ASTNode&& val){
    ASTNode x{NT_SETVAR,var,1};
    x.emplace(0,std::move(val));
    return x;
}
ASTNode getvar(std::uint64_t var){
    return {NT_GETVAR,var,0};
}
template<typename ...T>
ASTNode compound(T&& ...n){
    ASTNode x{NT_COMPOUND,0};
    (...,x.children().emplace_back(std::move(n)));
    return x;
}
