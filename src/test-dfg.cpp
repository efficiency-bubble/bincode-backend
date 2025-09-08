#include"test.hpp"
#include"dot.hpp"
#include<cppp/string.hpp>
#include<bbe/targets/dfg.hpp>
#include<bbe/inter/dfg.hpp>
#include<unordered_map>
#include<ranges>
#include<print>
using namespace std::literals;
std::unordered_map<std::uint32_t,cppp::sv> EXPLAIN{
    {0,u8"uint32"sv},
    {2,u8"pack"sv},
    {5,u8"arg32"sv},
    {9,u8"cmag"sv},
    {21,u8"fork"sv},
    {300,u8"proxy"sv},
    {301,u8"lctrl"sv},
    {std::numeric_limits<std::uint32_t>::max(),u8"env"sv}
};
int main(){
    Function example{nullptr,{},loop(cmag(FN_PRU32,pack(u32(1))))};
    targets::dfg::DataFlowGraph graph{example};
    DotFile df{u8"test/test.dot"s};
    for(const auto& node : graph.nodes()){
        std::uintptr_t nt = reinterpret_cast<std::uintptr_t>(&node);
        df.add_node(nt,EXPLAIN.at(node.operation())+u8";"s+cppp::tou8(std::to_string(node.primitive())));
        for(const auto& [i,parent] : std::views::enumerate(node.parents())){
            df.edge(nt,reinterpret_cast<std::uintptr_t>(parent),cppp::tou8(std::to_string(i)));
        }
    }
    df.close();
    [[maybe_unused]] inter::dfg::FunctionCall call{.argv{inter::uint32v{3}}};
    inter::dfg::eval(call,graph.root().env());
    return 0;
}
