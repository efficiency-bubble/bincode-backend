#include"test.hpp"
#include"dot.hpp"
#include<cppp/string.hpp>
#include<bbe/targets/dfg.hpp>
#include<bbe/inter/dfg.hpp>
#include<unordered_map>
#include<ranges>
#include<print>
int main(){
    ProjectEntitiesPool pep;
    std::uint32_t example_fn = pep.function_pool().emplace(nullptr,std::vector<const Type*>{});
    pep.function_pool()[example_fn].set(cmag(FN_PRU32,pack(u32(1))));
    inter::dfg::CompiledFunctionPool cpool{pep};
    const auto& graph = cpool.graph(example_fn);
    DotFile df{u8"test/test.dot"s};
    for(const auto& node : graph.nodes()){
        std::uintptr_t nt = reinterpret_cast<std::uintptr_t>(&node);
        df.add_node(nt,EXPLAIN.at(node.operation())+u8";"s+cppp::tou8(std::to_string(node.primitive())));
        for(const auto& [i,parent] : std::views::enumerate(node.parents())){
            df.edge(nt,reinterpret_cast<std::uintptr_t>(parent),cppp::tou8(std::to_string(i)));
        }
    }
    df.close();
    cpool.call(example_fn,{});
    return 0;
}
