#include"test.hpp"
#include<cppp/string.hpp>
#include<cppp/rtl.hpp>
#include<bbe/targets/rtl.hpp>
#include<cppp/print.hpp>
#include<unordered_map>
#include<ranges>
#include<print>
using namespace cppp::literals;
int main(){
    ProjectEntitiesPool pep;
    std::uint32_t example_fn = pep.function_pool().emplace(nullptr,std::vector<const Type*>{});
    pep.function_pool()[example_fn].set(comma(0uz,cmag(FN_PRU32,pack(u32(1))),cmag(FN_PRU32,pack(u32(2))),cmag(FN_PRU32,pack(u32(3)))));
    bbe::targets::rtl::Function fn{pep.function_pool()[example_fn]};
    
    for(const auto& ins : fn.instructions()){
        cppp::println<u8"{} {} {}"_ts>(bbe::targets::rtl::impl::stringify_enum(ins.opcode),ins.dst,ins.src);
    }
    return 0;
}
