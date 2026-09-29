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
    ErrorDatabase edb;
    
    const TypeInfo& ui32 = p.types()[TypeDatabase::T_UINT32];
    Function& example_fn = p.functions().emplace(u8"example"s,FunctionSignature{ui32,ui32,ui32});
    p.functions().emplace(u8"multiply_adjust"s,FunctionSignature{ui32,ui32,ui32});
    const Function& itadd = intrin(INTR_ADDU32,p);
    const Function& itsub = intrin(INTR_SUBU32,p);
    example_fn.set(
        fork(
            call(intrin(INTR_LEQU32,p),arg(0),u32(2)),
            call(itadd,u32(1),call(fn(1),arg(1),arg(1))),
            havevar(0,call(fn(0),call(itsub,arg(0),u32(1)),arg(1)),
            call(itadd,
                getvar(0),
                call(fn(0),call(itsub,arg(0),u32(2)),arg(1))
            ))
        )
    );
    example_fn.recalculate_types(p,edb);
    example_fn.recalculate_types(p,edb);
    if(!edb.empty()){
        cppp::println<u8"There are errors:"_ts>();
        for(const auto& [n,ev] : edb.all()){
            cppp::println<u8"· Node 0x{:x}:"_ts>(reinterpret_cast<std::uintptr_t>(n));
            for(const auto& e : ev){
                cppp::println<u8"  {}"_ts>(e.reason());
            }
        }
        return -1;
    }
    bbe::targets::x86::Function fn{example_fn,p};
    for(const std::byte b : fn.instructions()){
        cppp::print<u8"{:02x} "_ts>(static_cast<std::uint8_t>(b));
    }
    std::println();
    bbe::formats::elf::Elf elf;
    bbe::targets::x86::Program prog;
    prog.export_function(0,std::move(fn));
    prog.import_function(1,u8"multiply_adjust"sv);
    elf.add_text(prog);
    {
        cppp::BinaryFile outf{u8"test/example.o"s,std::ios_base::out|std::ios_base::binary|std::ios_base::trunc};
        outf.write(elf.encode());
    }
    std::system("objdump -d test/example.o && gcc test/test-main.cpp test/example.o -o test/example && test/example");
    return 0;
}
