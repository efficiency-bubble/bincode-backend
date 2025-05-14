#include<cppp/bfile.hpp>
#include<bbe/bbe.hpp>
#include<bbe/targets/x64.hpp>
#include<bbe/formats/elf.hpp>
#include<iostream>
using namespace bbe;
using namespace std::literals;
int main(){
    targets::x64::X64Program prog;
    ASTNode value{3,1,data_tag};
    value.setp(0,prog.import_symbol(u8"value"s,targets::x64::SymbolType::VARIABLE));
    ASTNode one{2,1,data_tag};
    one.setp(0,1);
    ASTNode five{2,1,data_tag};
    five.setp(0,5);
    ASTNode subv1{1,2};
    subv1.emplace(0,std::move(value));
    subv1.emplace(1,std::move(one));
    ASTNode subv15{1,2};
    subv15.emplace(0,std::move(subv1));
    subv15.emplace(1,std::move(five));
    ASTNode ret{0,1};
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
