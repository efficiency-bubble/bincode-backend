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
        wt(f,u8"[label=\""sv);
        wt(f,label);
        wt(f,u8"\"] "sv);
    }
    public:
        DotFile(std::filesystem::path path,bool directed=true) : f(path,std::ios_base::out|std::ios_base::trunc){
            if(directed){
                wt(f,u8"digraph{"sv);
            }else{
                wt(f,u8"graph{"sv);
            }
        }
        void add_node(std::uint64_t id,cppp::sv lb){
            wt(f,cppp::tou8(std::to_string(id)));
            label(lb);
        }
        void edge(std::uint64_t p,std::uint64_t q,cppp::sv lb){
            wt(f,cppp::tou8(std::to_string(p)));
            if(directed){
                wt(f,u8"->"sv);
            }else{
                wt(f,u8"--"sv);
            }
            wt(f,cppp::tou8(std::to_string(q)));
            label(lb);
        }
        void close(){
            f.writeb(u8'}');
            f.close();
        }
};
