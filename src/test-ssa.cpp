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
int main(){
    ASTNode x{NT_ARG32,1,data_tag};
    x.setp(0,0);
    ASTNode y{NT_ARG32,1,data_tag};
    y.setp(0,1);
    ASTNode addxy{NT_ADD,2};
    addxy.emplace(0,std::move(x));
    addxy.emplace(1,std::move(y));
    ASTNode retaxy{NT_RET,1};
    retaxy.emplace(0,std::move(addxy));
    ASTNode zero{NT_U32,1,data_tag};
    zero.setp(0,0);
    ASTNode retzero{NT_RET,1};
    retzero.emplace(0,std::move(zero));
    ASTNode flag{NT_ARGB,1,data_tag};
    flag.setp(0,2);
    ASTNode branch{NT_FORK,3};
    branch.emplace(0,std::move(flag));
    branch.emplace(1,std::move(retaxy));
    branch.emplace(2,std::move(retzero));
    Function example{nullptr,{},std::move(branch)};
    targets::ssa::ProcedureIC prog;
    prog.compile(example);
    for(const auto& [i,b] : prog.blocks() | std::ranges::views::enumerate){
        std::cout << "Block #"sv << i << '\n';
        for(const targets::ssa::Instruction& ins : b.instructions()){
            std::cout << "    "sv << cppp::cview(ins.debug()) << '\n';
        }
    }
    std::cout.flush();
    return 0;
}
