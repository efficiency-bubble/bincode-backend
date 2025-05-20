#include<cppp/bfile.hpp>
#include<bbe/bbe.hpp>
#include<bbe/targets/x64.hpp>
#include<bbe/formats/elf.hpp>
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
constexpr static std::uint64_t NT_SYM32 = 100;
constexpr static std::uint64_t NT_SYM64 = 101;
int main(){
    targets::x64::X64Program prog;
    ASTNode n{NT_SYM32,1,data_tag};
    n.setp(0,prog.import_symbol(u8"n"s,targets::x64::SymbolType::VARIABLE));
    ASTNode x{NT_ARG32,1,data_tag};
    x.setp(0,0);
    ASTNode addnx{NT_ADD,2};
    addnx.emplace(0,std::move(n));
    addnx.emplace(1,std::move(x));
    ASTNode ret{NT_RET,1};
    ret.emplace(0,std::move(addnx));
    Function example{nullptr,{},std::move(ret)};
    prog.start_export(u8"plus_n"s);
    prog.compile(example);
    cppp::BinaryFile bf{u8"test.o"sv,std::ios::binary | std::ios::out};
    formats::elf::Elf elf;
    elf.add_text(prog);
    bf.write(elf.encode());
    return 0;
}
