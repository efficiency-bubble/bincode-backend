#pragma once
#include<bbe/bbe.hpp>
using namespace bbe;
constexpr static std::uint64_t NT_U32 = 0;
constexpr static std::uint64_t NT_U64 = 1;
constexpr static std::uint64_t NT_PACK = 2;
constexpr static std::uint64_t NT_ARG32 = 5;
constexpr static std::uint64_t NT_ARG64 = 6;
constexpr static std::uint64_t NT_RET = 7;
constexpr static std::uint64_t NT_CALL = 8;
constexpr static std::uint64_t NT_FN = 9;
constexpr static std::uint64_t NT_SETVAR = 10;
constexpr static std::uint64_t NT_GETVAR = 11;
constexpr static std::uint64_t NT_FORK = 21;
constexpr static std::uint64_t NT_ARGB = 22;
constexpr static std::uint64_t NT_COMPOUND = 64;
constexpr static std::uint64_t NT_SYM32 = 100;
constexpr static std::uint64_t NT_SYM64 = 101;

constexpr static std::uint32_t FN_ADD = 10;
ASTNode arg32(std::uint32_t id){
    return {NT_ARG32,id,0};
}
ASTNode argb(std::uint32_t id){
    return {NT_ARGB,id,0};
}
ASTNode pack(ASTNode&& lhs,ASTNode&& rhs){
    ASTNode x{NT_PACK,2};
    x.emplace(0,std::move(lhs));
    x.emplace(1,std::move(rhs));
    return x;
}
ASTNode call(ASTNode&& fn,ASTNode&& arg){
    ASTNode x{NT_CALL,2};
    x.emplace(0,std::move(fn));
    x.emplace(1,std::move(arg));
    return x;
}
ASTNode fn(std::uint64_t id){
    return {NT_FN,id,0};
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
