#include"test.hpp"
#include"dot.hpp"
#include<cppp/string.hpp>
#include<bbe/targets/dfg.hpp>
#include<bbe/inter/dfg.hpp>
#include<unordered_map>
#include<ranges>
#include<print>
template<typename Cl,typename Other>
void show_clobber(DotFile& df,const Cl& clob,cppp::sv s){
    std::uintptr_t id = reinterpret_cast<std::uintptr_t>(&clob);
    df.add_node(id,s);
    for(const auto& [i,vari] : std::views::enumerate(clob.sequence)){
        if(std::holds_alternative<const targets::dfg::DataNode*>(vari)){
            df.edge(id,reinterpret_cast<std::uintptr_t>(std::get<const targets::dfg::DataNode*>(vari)),cppp::tou8(std::to_string(i)));
        }else{
            show_clobber<Other,Cl>(df,std::get<Other>(vari),s);
        }
    }
}
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
    for(const auto& [i,clob] : graph.clobbers()){
        show_clobber<targets::dfg::SequentialClobber,targets::dfg::ParallelClobber>(df,clob,u8"clob_"s+cppp::tou8(std::to_string(i)));
    }
    df.close();
    return 0;
}
