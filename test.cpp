#include<cppp/bfile.hpp>
#include<bbe/bbe.hpp>
#include<iostream>
using namespace bbe;
using namespace std::literals;
int main(){
    Text text;
    targets::Defaultx64 compiler;
    ASTNode seven{2,1,data_tag};
    seven.setp(0,7);
    ASTNode one{2,1,data_tag};
    one.setp(0,1);
    ASTNode five{2,1,data_tag};
    five.setp(0,5);
    ASTNode sub71{1,2};
    sub71.emplace(0,std::move(seven));
    sub71.emplace(1,std::move(one));
    ASTNode sub715{1,2};
    sub715.emplace(0,std::move(sub71));
    sub715.emplace(1,std::move(five));
    ASTNode ret{0,1};
    ret.emplace(0,std::move(sub715));
    Function main{u8"main"s,nullptr,{},std::move(ret)};
    compiler.compile(main,text);
    cppp::BinaryFile bf{u8"test.bin"sv,std::ios::binary | std::ios::out};
    bf.write(text.text());
    return 0;
}
