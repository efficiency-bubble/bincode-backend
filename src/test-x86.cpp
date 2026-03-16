#include"test.hpp"
#include<cppp/static-functor.hpp>
#include<bbe/targets/x86.hpp>
#include<bbe/formats/elf.hpp>
#include<cppp/string.hpp>
#include<cppp/bfile.hpp>
#include<cppp/print.hpp>
#include<unordered_map>
#include<cstdlib>
#include<dlfcn.h>
#include<memory>
#include<ranges>
#include<print>
using namespace cppp::literals;
int main(){
    ProjectEntitiesPool pep;
    std::uint32_t example_fn = pep.function_pool().emplace(nullptr,std::vector<const Type*>{});
    pep.function_pool()[example_fn].set(
        cmag(FN_EQ32,u32(1024),cmag(FN_SUB32,u32(1025),u32(1)))
    );
    bbe::targets::x86::Function fn{pep.function_pool()[example_fn]};
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
    std::system("gcc -shared test/example.o -o test/example.so");
    
    std::unique_ptr<void,cppp::static_functor<dlclose>> dl{dlopen("test/example.so",RTLD_LAZY)};
    cppp::println<u8"Function returns: {}"_ts>(std::bit_cast<bool(*)()>(dlsym(dl.get(),"example"))());
    return 0;
}
