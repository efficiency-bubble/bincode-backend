#include<bbe/ast.hpp>
#include<bbe/targets/x86.hpp>
#include<bbe/formats/elf.hpp>
#include<cppp/format.hpp>
#include<cppp/string.hpp>
#include<cppp/rtl.hpp>
#include<iostream>
#include<array>
using namespace std::literals;
using namespace cppp::literals;
cppp::str cstreamtou8(const char* s){
    cppp::str buf;
    while(char c=*s++){
        buf.push_back(static_cast<char8_t>(c));
    }
    return buf;
}
[[noreturn]] void error(cppp::sv msg){
    std::cerr << "bcc: "sv << cppp::cview(msg) << "\ntype 'bcc -h' for help.\n"sv;
    std::exit(-1);
}
[[noreturn]] void usage(int status){
    std::cerr << "Usage: bcc <infile> -o <outfile>"sv << '\n';
    std::exit(status);
}
class Argv{
    char const* const* p;
    public:
        Argv(char const* const* p) : p(p){}
        bool has_more() const{
            return *p;
        }
        cppp::str next(){
            if(!*p) throw std::logic_error("Argv::next(): unexpected end of argument list");
            return cstreamtou8(*p++);
        }
};
int main(int,char* argv[]){
    std::array<cppp::str,2uz> args;
    {
        auto scan = args.begin();
        Argv arg_scanner{argv+1uz};
        while(arg_scanner.has_more()){
            cppp::str arg{arg_scanner.next()};
            if(arg.starts_with(u8'-')){
                cppp::sv opt = arg.subview(1uz);
                if(opt == u8"h"sv){
                    usage(0);
                }else{
                    error(cppp::format<u8"unknown option: {}"_ts>(arg));
                }
            }else{
                if(scan == args.end()) error(u8"too many arguments"sv);
                *scan++ = std::move(arg);
            }
        }
        if(scan == args.begin()) usage(-1);
        if(scan != args.end()) error(u8"missing operand"sv);
    }
    cppp::bytes indata;
    try{
        cppp::read_file(indata,args[0uz]);
    }catch(const cppp::operation_failed&){
        error(u8"couldn't read input file"sv);
    }
    bbe::formats::elf::Elf elf;
    bbe::ProjectEntitiesPool pep{cppp::rtl<cppp::frozen_byte_view>(indata)};
    elf.add_text(pep);
    cppp::BinaryFile outfile{args[2uz],std::ios_base::out|std::ios_base::trunc};
    outfile.write(elf.encode());
    return 0;
}
