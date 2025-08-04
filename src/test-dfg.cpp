#include"test.hpp"
#include<cppp/bfile.hpp>
#include<cppp/string.hpp>
#include<bbe/targets/dfg.hpp>
#include<bbe/inter/dfg.hpp>
#include<unordered_map>
#include<print>
using namespace std::literals;
void wt(cppp::BinaryFile& f,cppp::sv s){
    f.write(cppp::frozenbuffer(reinterpret_cast<const std::byte*>(s.data()),s.size()));
}
class DotFile{
    bool directed;
    cppp::BinaryFile f;
    public:
        DotFile(std::filesystem::path path,bool directed=true) : f(path,std::ios_base::out|std::ios_base::trunc){
            if(directed){
                wt(f,u8"digraph{"sv);
            }else{
                wt(f,u8"graph{"sv);
            }
        }
        void add_node(std::uint64_t id,cppp::sv label){
            wt(f,cppp::tou8(std::to_string(id)));
            wt(f,u8"[label=\""sv);
            wt(f,label);
            wt(f,u8"\"] "sv);
        }
        void edge(std::uint64_t p,std::uint64_t q){
            wt(f,cppp::tou8(std::to_string(p)));
            if(directed){
                wt(f,u8"->"sv);
            }else{
                wt(f,u8"--"sv);
            }
            wt(f,cppp::tou8(std::to_string(q)));
            f.writeb(u8' ');
        }
        void close(){
            f.writeb(u8'}');
            f.close();
        }
};
std::unordered_map<std::uint32_t,cppp::sv> EXPLAIN{
    {0,u8"uint32"sv},
    {2,u8"pack"sv},
    {5,u8"arg32"sv},
    {9,u8"cmag"sv},
    {21,u8"fork"sv}
};
int main(){
    Function example{nullptr,{},fork(arg32(2),arg32(0),cmag(FN_ADD,pack(arg32(0),arg32(1))))};
    targets::dfg::DataFlowGraph graph{example};
    DotFile df{u8"test/test.dot"s};
    for(const auto& node : graph.nodes()){
        std::uintptr_t nt = reinterpret_cast<std::uintptr_t>(&node);
        df.add_node(nt,EXPLAIN[node.operation()]+u8";"s+cppp::tou8(std::to_string(node.primitive())));
        for(const auto& parent : node.parents()){
            df.edge(nt,reinterpret_cast<std::uintptr_t>(&parent.node()));
        }
    }
    df.close();
    inter::dfg::FunctionCall call{.argv{inter::uint32v{3},inter::uint32v{4},inter::boolv{true}}};
    std::println("{}"sv,inter::dfg::eval(call,graph.root()).get<inter::uint32v>().value);
    return 0;
}
