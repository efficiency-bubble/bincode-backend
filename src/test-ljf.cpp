#include"test.hpp"
#include<cppp/bfile.hpp>
#include<bbe/targets/ljf.hpp>
#include<bbe/inter/ljf.hpp>
#include<iostream>
#include<ranges>
using namespace std::literals;
int main(){
    Function example{nullptr,{},compound(
        ret(call(fn(FN_ADD),pack(arg32(0),arg32(1))))
    )};
    targets::ljf::ProcedureIC prog{example};
    std::vector<std::uint32_t> l = prog.labels();
    std::ranges::reverse(l);
    std::uint32_t i = 0;
    std::uint32_t lc = 0;
    for(const auto& ins : prog.instructions()){
        if(!l.empty()&&i==l.back()){
            std::cout << std::setw(2) << lc++ << std::setw(1) << ": "sv;
            l.pop_back();
        }else{
            std::cout << "    "sv;
        }
        std::cout << cppp::cview(ins.debug()) << '\n';
        ++i;
    }
    std::cout.flush();
    inter::ljf::GlobalEnvironment env;
    std::cout << inter::ljf::run(env,prog,{inter::uint32v(3),inter::uint32v(5)}).get<inter::uint32v>().value << std::endl;
    return 0;
}
