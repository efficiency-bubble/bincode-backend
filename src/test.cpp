#include<cppp/bfile.hpp>
#include<bbe/bbe.hpp>
#include<bbe/targets/x64.hpp>
#include<bbe/formats/elf.hpp>
#include<iostream>
using namespace bbe;
using namespace std::literals;
int main(){
    targets::x64::X64Compiler compiler;
    ASTNode seven{2,1,data_tag};
    seven.setp(0,7);
    ASTNode one{2,1,data_tag};
    one.setp(0,1);
    ASTNode five{2,1,data_tag};
    five.setp(0,5);
    ASTNode sub71{1,2};
    sub71.emplace(0,std::move(seven));
    sub71.emplace(1,std::move(one));
    ASTNode sub715{1,2};
    sub715.emplace(0,std::move(sub71));
    sub715.emplace(1,std::move(five));
    ASTNode ret{0,1};
    ret.emplace(0,std::move(sub715));
    Function example{nullptr,{},std::move(ret)};
    Text text;
    text.start_function(u8"example"s);
    compiler.compile(example,text);
    cppp::BinaryFile bf{u8"test.o"sv,std::ios::binary | std::ios::out};
    formats::elf::Elf elf;
    elf.add_text(text);
    bf.write(elf.encode());
    return 0;
}
