#include<cppp/bfile.hpp>
#include<bbe/bbe.hpp>
#include<iostream>
using namespace bbe;
using namespace std::literals;
using cppp::ptr;
int main(){
    Text text;
    FunctionCompilationContext fcc{text};
    Return ret{
        ptr<Subi32>::construct(
            ptr<Subi32>::construct(
                ptr<Constanti32>::construct(7),
                ptr<Constanti32>::construct(1)
            ),
            ptr<Constanti32>::construct(5)
        )
    };
    ret.compile(fcc);
    cppp::BinaryFile bf{u8"D:/Desktop/test.bin"sv,std::ios::binary | std::ios::out};
    bf.write(text.text());
    return 0;
}
