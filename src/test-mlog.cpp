#include"test.hpp"
#include<cppp/bfile.hpp>
#include<bbe/targets/mlog.hpp>
#include<iostream>
#include<ranges>
using namespace std::literals;
int main(){
    Function example{nullptr,{},compound(
        cmag(FN_ADD,pack(u32(0),u32(1)))
    )};
    std::cout << cppp::tocs(targets::mlog::compile(targets::ssa::ProcedureIC(example))) << std::endl;
    return 0;
}
