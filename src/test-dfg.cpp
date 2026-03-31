#include"test.hpp"
#include"dot.hpp"
#include<cppp/string.hpp>
#include<bbe/targets/dfg.hpp>
#include<bbe/inter/dfg.hpp>
#include<unordered_map>
#include<ranges>
#include<print>
void _encode_clist(DotFile& df,const targets::dfg::Clobbers& cb,
std::uint64_t& aux,cppp::str parent){
    cppp::str label;
    bool first = true;
    for(const auto& v : cb.sequence()){
        cppp::str thisnode{u8"seqc"s};
        thisnode.append(cppp::tou8(std::to_string(aux++)));
        switch(v.index()){
            case v.index_of<targets::dfg::Clobbers>:
                _encode_clist(df,v.get<targets::dfg::Clobbers>(),aux,thisnode);
                if(v.get<targets::dfg::Clobbers>().ordering()){
                    label = u8"seq"s;
                }else{
                    label = u8"par"s;
                }
                break;
            case v.index_of<targets::dfg::Fork>: {
                const auto& fk = v.get<targets::dfg::Fork>();
                label = u8"fork"s;
                _encode_clist(df,fk.left(),aux,thisnode);
                _encode_clist(df,fk.right(),aux,thisnode);
                break;
            }
            case v.index_of<const targets::dfg::DataNode*>: {
                df.write(u8"} "sv);
                df.edge(thisnode,cppp::tou8(std::to_string(reinterpret_cast<std::uintptr_t>(v.get<const targets::dfg::DataNode*>()))),u8":dashed\",color=\"blue\",arrowhead=\"none"sv);
                label = u8"node"s;
                df.write(u8"subgraph clobs{"sv);
                break;
            }
        }
        df.add_node(thisnode,label);
        if(!parent.empty())
            df.edge(parent,thisnode,first?u8":dashed\",color=\"red"sv:u8""sv);
        first = false;
        parent = std::move(thisnode);
    }
}
void encode_clist(DotFile& df,const targets::dfg::Clobbers& cb){
    std::uint64_t aux = 0;
    _encode_clist(df,cb,aux,u8""s);
}
int main(){
    ProjectEntitiesPool pep;
    std::uint32_t example_fn = pep.function_pool().emplace(nullptr,std::vector<const TypeLayout*>{});
    pep.function_pool()[example_fn].set(comma(2uz,cmag(FN_PRU32,pack(u32(1))),cmag(FN_PRU32,pack(u32(2))),cmag(FN_PRU32,pack(u32(3))),cmag(FN_PRU32,pack(u32(3))),cmag(FN_PRU32,pack(u32(3))),cmag(FN_PRU32,pack(u32(3)))));
    inter::dfg::CompiledFunctionPool cpool{pep};
    const auto& graph = cpool.graph(example_fn);
    DotFile df{u8"test/test.dot"s};
    df.write(u8"newrank=true subgraph main{ cluster=true "sv);
    for(const auto& node : graph.nodes()){
        std::uintptr_t nt = reinterpret_cast<std::uintptr_t>(&node);
        df.add_node(nt,EXPLAIN.at(node.operation())+u8";"s+cppp::tou8(std::to_string(node.primitive())));
        for(const auto& [i,parent] : std::views::enumerate(node.parents())){
            df.edge(nt,reinterpret_cast<std::uintptr_t>(parent),cppp::tou8(std::to_string(i)));
        }
    }
    df.write(u8"} subgraph clobs{ cluster=true "sv);
    encode_clist(df,graph.clobbers());
    df.write(u8"}"sv);
    df.close();
    cpool.call(example_fn,{});
    return 0;
}
