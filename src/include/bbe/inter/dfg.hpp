#pragma once
#include"../targets/dfg.hpp"
#include"../entity_pool.hpp"
#include<unordered_map>
#include"value.hpp"
#include<iostream>
namespace bbe::inter::dfg::impl{
    using namespace bbe::inter::impl;
    class CompiledFunctionPool{
        std::unordered_map<ProjectEntitiesPool::index_type,targets::dfg::DataFlowGraph> pool;
        public:
            CompiledFunctionPool(const ProjectEntitiesPool& pep){
                for(const auto& [ind,fn] : pep.function_pool()){
                    pool.try_emplace(ind,fn);
                }
            }
            const targets::dfg::DataFlowGraph& graph(ProjectEntitiesPool::index_type i) const{
                return pool.at(i);
            }
            Value call(ProjectEntitiesPool::index_type,const std::vector<Value>&) const;
    };
}
namespace bbe::inter::dfg{
    BBE_EXPORT CompiledFunctionPool;
}
