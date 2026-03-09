#include"test.hpp"
#include<cppp/bfile.hpp>
#include<bbe/targets/mlog.hpp>
#include<iostream>
#include<ranges>
using namespace std::literals;
int main(){
    Function example{nullptr,{},compound(
        cmag(1550,fork(cbool(true),cmag(FN_ADD32,pack(u32(2),u32(3))),u32(2))),
        cmag(1600,pack()),
        cmag(2520,pack())
    )};
    std::cout << cppp::tocs(targets::mlog::ProcedureIC(targets::ljf::ProcedureIC(example)).encode()) << std::endl;
    return 0;
}
