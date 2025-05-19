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
constexpr static std::uint64_t NT_RET = 5;
constexpr static std::uint64_t NT_SYM32 = 100;
constexpr static std::uint64_t NT_SYM64 = 101;
int main(){
    targets::x64::X64Program prog;
    ASTNode value{NT_SYM32,1,data_tag};
    value.setp(0,prog.import_symbol(u8"value"s,targets::x64::SymbolType::VARIABLE));
    ASTNode one{NT_U32,1,data_tag};
    one.setp(0,1);
    ASTNode five{NT_U32,1,data_tag};
    five.setp(0,5);
    ASTNode subv1{NT_SUB,2};
    subv1.emplace(0,std::move(value));
    subv1.emplace(1,std::move(one));
    ASTNode subv15{NT_SUB,2};
    subv15.emplace(0,std::move(subv1));
    subv15.emplace(1,std::move(five));
    ASTNode ret{NT_RET,1};
    ret.emplace(0,std::move(subv15));
    Function example{nullptr,{},std::move(ret)};
    prog.start_export(u8"example"s);
    prog.compile(example);
    cppp::BinaryFile bf{u8"test.o"sv,std::ios::binary | std::ios::out};
    formats::elf::Elf elf;
    elf.add_text(prog);
    bf.write(elf.encode());
    return 0;
}
