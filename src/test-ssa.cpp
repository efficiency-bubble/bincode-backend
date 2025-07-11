#include"test.hpp"
#include<cppp/bfile.hpp>
#include<bbe/targets/ssa.hpp>
#include<iostream>
#include<ranges>
using namespace std::literals;
void print_nvtable(const std::unordered_map<std::uint32_t,std::uint32_t>& t){
    std::cout << '(';
    for(const auto& [n,v] : t){
        std::cout << " n"sv << n << "=v"sv << v;
    }
    std::cout <<  " )"sv;
}
int main(){
    Function example{nullptr,{},compound(
        ret(call(fn(FN_ADD),pack(arg32(0),arg32(1))))
    )};
    targets::ssa::ProcedureIC prog;
    prog.compile(example);
    for(const auto& [i,b] : prog.blocks() | std::ranges::views::enumerate){
        std::cout << "Block # "sv << i << ' ';
        print_nvtable(b.imports());
        std::cout << '\n';
        for(const targets::ssa::Instruction& ins : b.instructions()){
            std::cout << "    "sv << cppp::cview(ins.debug()) << '\n';
        }
        print_nvtable(b.nametable());
        if(b.retcond()==targets::ssa::BasicBlock::NCOND){
            if(!b.retlocs().empty()){
                std::cout << " -> b"sv << b.retlocs()[0];
            }
        }else{
            std::cout << " -> v"sv << b.retcond() << " ? b"sv << b.retlocs()[0] << " : b"sv << b.retlocs()[1];
        }
        std::cout << '\n';
    }
    std::cout.flush();
    return 0;
}
