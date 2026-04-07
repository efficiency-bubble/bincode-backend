#pragma once
#include"../project_entity_pool.hpp"
namespace bbe::inter::impl{
    template<typename T>
    class CompiledFunctionPool{
        std::unordered_map<ProjectEntitiesPool::index_type,T> pool;
        public:
            CompiledFunctionPool(const ProjectEntitiesPool& pep){
                for(const auto& [ind,fn] : pep.functions()){
                    pool.try_emplace(ind,fn);
                }
            }
            const T& function(ProjectEntitiesPool::index_type i) const{
                return pool.at(i);
            }
    };
}
