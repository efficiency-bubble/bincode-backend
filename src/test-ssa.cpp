#include<cppp/bfile.hpp>
#include<bbe/bbe.hpp>
#include<bbe/targets/ssa.hpp>
#include<iostream>
#include<ranges>
using namespace bbe;
using namespace std::literals;
constexpr static std::uint64_t NT_U32 = 0;
constexpr static std::uint64_t NT_U64 = 1;
constexpr static std::uint64_t NT_ADD = 2;
constexpr static std::uint64_t NT_SUB = 3;
constexpr static std::uint64_t NT_UMUL = 4;
constexpr static std::uint64_t NT_ARG32 = 5;
constexpr static std::uint64_t NT_ARG64 = 6;
constexpr static std::uint64_t NT_RET = 7;
constexpr static std::uint64_t NT_CALL = 8;
constexpr static std::uint64_t NT_FORK = 21;
constexpr static std::uint64_t NT_ARGB = 22;
constexpr static std::uint64_t NT_SYM32 = 100;
constexpr static std::uint64_t NT_SYM64 = 101;
ASTNode arg32(std::uint32_t id){
    return {NT_ARG32,id,0};
}
ASTNode argb(std::uint32_t id){
    return {NT_ARGB,id,0};
}
ASTNode add(ASTNode&& lhs,ASTNode&& rhs){
    ASTNode x{NT_ADD,2};
    x.emplace(0,std::move(lhs));
    x.emplace(1,std::move(rhs));
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
int main(){
    Function example{nullptr,{},fork(
        argb(2),
        ret(add(arg32(0),arg32(1))),
        ret(arg32(0))
    )};
    targets::ssa::ProcedureIC prog;
    prog.compile(example);
    for(const auto& [i,b] : prog.blocks() | std::ranges::views::enumerate){
        std::cout << "Block # "sv << i << " ("sv;
        for(const auto& [n,v] : b.imports()){
            std::cout << " n"sv << n << "=v"sv << v;
        }
        std::cout <<  ")\n"sv;
        for(const targets::ssa::Instruction& ins : b.instructions()){
            std::cout << "    "sv << cppp::cview(ins.debug()) << '\n';
        }
    }
    std::cout.flush();
    return 0;
}
