#pragma once
#include"../project_entity_pool.hpp"
namespace bbe::inter::impl{
    template<typename T>
    class CompiledFunctionPool{
        std::unordered_map<func_id,T> pool;
        public:
            CompiledFunctionPool(const ProjectEntitiesPool& pep){
                for(const auto& fn : pep.functions()){
                    if(!fn.is_intrin()){
                        pool.try_emplace(fn.index(),pep,fn);
                    }
                }
            }
            const T& function(func_id i) const{
                return pool.at(i);
            }
    };
}
