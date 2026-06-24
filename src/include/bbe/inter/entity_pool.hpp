#pragma once
#include"../project_entity_pool.hpp"
namespace bbe::inter::impl{
    template<typename T>
    class CompiledFunctionPool{
        std::unordered_map<func_id,T> pool;
        public:
            CompiledFunctionPool(const ProjectEntitiesPool& pep){
                for(const auto& fn : pep.functions()){
                    pool.try_emplace(fn.index(),fn);
                }
            }
            const T& function(func_id i) const{
                return pool.at(i);
            }
    };
}
