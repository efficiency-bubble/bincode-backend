#include"test.hpp"
#include<cppp/bfile.hpp>
#include<bbe/targets/ljf.hpp>
#include<bbe/inter/ljf.hpp>
#include<iostream>
#include<ranges>
using namespace std::literals;
int main(){
    Function example{nullptr,{},compound(
        ret(fork(arg32(2),arg32(0),cmag(FN_ADD32,pack(arg32(0),arg32(1)))))
    )};
    targets::ljf::ProcedureIC prog{example};
    auto l = prog.labels().begin();
    std::uint32_t lc = 0;
    for(auto it=prog.instructions().begin();it!=prog.instructions().end();++it){
        if(l != prog.labels().end()&&it==*l){
            std::cout << std::setw(2) << lc++ << std::setw(1) << ": "sv;
            ++l;
        }else{
            std::cout << "    "sv;
        }
        std::cout << cppp::cview(it->debug()) << '\n';
    }
    std::cout.flush();
    inter::ljf::GlobalEnvironment env;
    std::cout << inter::ljf::run(env,prog,{inter::uint32v(3),inter::uint32v(5),inter::boolv(true)}).get<inter::uint32v>().value << std::endl;
    return 0;
}
