#include<cppp/bfile.hpp>
#include<bbe/bbe.hpp>
#include<bbe/targets/ssa.hpp>
#include<iostream>
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
    ASTNode ret{NT_RET,1};
    ret.emplace(0,std::move(addxy));
    Function example{nullptr,{},std::move(ret)};
    targets::ssa::ProcedureIC prog;
    prog.compile(example);
    for(const auto& ins : prog.instructions()){
        std::cout << cppp::cview(ins.debug()) << std::endl;
    }
    return 0;
}
