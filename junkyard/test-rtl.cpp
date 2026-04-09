#include"test.hpp"
#include<cppp/string.hpp>
#include<cppp/rtl.hpp>
#include<bbe/inter/rtl.hpp>
#include<cppp/print.hpp>
#include<unordered_map>
#include<ranges>
#include<print>
using namespace cppp::literals;
int main(){
    ProjectEntitiesPool pep;
    std::uint32_t example_fn = pep.function_pool().emplace(nullptr,std::vector<const TypeInfo*>{});
    pep.function_pool()[example_fn].set(fibonacci());
    bbe::inter::rtl::CompiledFunctionPool fn{pep};
    
    std::size_t i = 0uz;
    for(const auto& ins : fn.function(example_fn).instructions()){
        cppp::println<u8"{} {} {} {}"_ts>(i++,bbe::targets::rtl::impl::stringify_enum(ins.opcode),ins.dst,ins.src);
    }
    cppp::str ret;
    bbe::inter::stringify(fn.call(example_fn,{bbe::inter::uint32v{20}}),ret);
    cppp::println<u8"{}"_ts>(ret);
    return 0;
}
