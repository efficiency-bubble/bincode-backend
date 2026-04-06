#pragma once
#include"entity_pool.hpp"
#include"type.hpp"
namespace bbe::impl{
    class ProjectEntitiesPool{
        public:
            using index_type = std::uint32_t;
        private:
            EntityPool<Function,index_type> fn_p;
            TypeDatabase td;
        public:
            const TypeDatabase& types() const{
                return td;
            }
            TypeDatabase& types(){
                return td;
            }
            EntityPool<Function,index_type>& function_pool(){
                return fn_p;
            }
            const EntityPool<Function,index_type>& function_pool() const{
                return fn_p;
            }
    };
}
namespace bbe{
    BBE_EXPORT ProjectEntitiesPool;
}
