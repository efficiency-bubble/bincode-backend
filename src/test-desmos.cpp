#include"test.hpp"
#include<cppp/string.hpp>
#include<bbe/targets/desmos.hpp>
#include<cstdio>
int main(){
    ProjectEntitiesPool pep;
    std::uint32_t example_fn = pep.function_pool().emplace(nullptr,std::vector<const Type*>{});
    pep.function_pool()[example_fn].set(fork(cbool(false),u32(1),u32(2)));
    cppp::str formulae;
    bbe::targets::desmos::compile(formulae,pep.function_pool()[example_fn],u8"foo"sv);
    std::fwrite(formulae.data(),formulae.size(),1,stdout);
    return 0;
}
