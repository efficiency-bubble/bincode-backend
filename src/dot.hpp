#pragma once
#include<cppp/bfile.hpp>
#include<cppp/string.hpp>
using namespace std::literals;
void wt(cppp::BinaryFile& f,cppp::sv s){
    f.write(cppp::frozenbuffer(reinterpret_cast<const std::byte*>(s.data()),s.size()));
}
class DotFile{
    bool directed;
    cppp::BinaryFile f;
    void label(cppp::sv label){
        if(label.empty()){
            write(u8" "sv);
            return;
        }
        cppp::str marker{u8"label"s};
        if(label.starts_with(u8':')){
            marker = u8"style";
            label.remove_prefix(1uz);
        }
        write(u8"["s+marker+u8"=\""sv);
        write(label);
        write(u8"\"] "sv);
    }
    public:
        DotFile(std::filesystem::path path,bool directed=true) : f(path,std::ios_base::out|std::ios_base::trunc){
            if(directed){
                write(u8"digraph{"sv);
            }else{
                write(u8"graph{"sv);
            }
        }
        void write(cppp::sv text){
            wt(f,text);
        }
        void add_node(cppp::sv id,cppp::sv lb){
            write(id);
            label(lb);
        }
        void add_node(std::uint64_t id,cppp::sv lb){
            add_node(cppp::tou8(std::to_string(id)),lb);
        }
        void edge(cppp::sv p,cppp::sv q,cppp::sv lb){
            write(p);
            if(directed){
                write(u8"->"sv);
            }else{
                write(u8"--"sv);
            }
            write(q);
            label(lb);
        }
        void edge(std::uint64_t p,std::uint64_t q,cppp::sv lb){
            edge(cppp::tou8(std::to_string(p)),cppp::tou8(std::to_string(q)),lb);
        }
        void close(){
            f.writeb(u8'}');
            f.close();
        }
};
