#include"test.hpp"
#include<bbe/targets/x86.hpp>
#include<bbe/formats/elf.hpp>
#include<cppp/string.hpp>
#include<cppp/bfile.hpp>
#include<cppp/print.hpp>
#include<unordered_map>
#include<cstdlib>
#include<memory>
#include<ranges>
#include<print>
using namespace cppp::literals;
int main(){
    ProjectEntitiesPool p;
    TypeDatabase tdb;
    ErrorDatabase edb;
    std::uint32_t example_fn = p.functions().emplace(FunctionSignature{TypeDatabase::T_UINT32,TypeDatabase::T_UINT32});
    p.functions()[example_fn].set(fibonacci());
    p.functions()[example_fn].recalculate_types(p,edb);
    bbe::targets::x86::Function fn{p.functions()[example_fn],tdb};
    for(const std::byte b : fn.instructions()){
        cppp::print<u8"{:02X} "_ts>(static_cast<std::uint8_t>(b));
    }
    std::println();
    bbe::formats::elf::Elf elf;
    bbe::targets::x86::Program prog;
    prog.export_function(u8"example"s,std::move(fn));
    elf.add_text(prog);
    {
        cppp::BinaryFile outf{u8"test/example.o"s,std::ios_base::out|std::ios_base::binary|std::ios_base::trunc};
        outf.write(elf.encode());
    }
    std::system("objdump -d test/example.o && gcc test/test-main.cpp test/example.o -o test/example && test/example");
    return 0;
}
