#include"test.hpp"
#include<cppp/string.hpp>
#include<bbe/targets/dfg.hpp>
#include<bbe/inter/dfg.hpp>
#include<unordered_map>
#include<ranges>
#include<print>
int main(){
    ProjectEntitiesPool pep;
    std::uint32_t example_fn = pep.function_pool().emplace(nullptr,std::vector<const Type*>{});
    std::uint32_t example_fn_2 = pep.function_pool().emplace(nullptr,std::vector<const Type*>{});
    pep.function_pool()[example_fn].set(comma(1uz,cmag(FN_PRU32,pack(u32(1))),cmag(FN_CALL,pack(fn(example_fn_2),pack()))));
    pep.function_pool()[example_fn_2].set(cmag(FN_CALL,pack(fn(example_fn),pack())));
    inter::dfg::CompiledFunctionPool cpool{pep};
    cpool.call(example_fn_2,{});
    return 0;
}
