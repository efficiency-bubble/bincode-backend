#pragma once
#include"function.hpp"
#include"type.hpp"
namespace bbe::impl{
    class ProjectEntitiesPool{
        FunctionDatabase fd;
        TypeDatabase td;
        public:
            const TypeDatabase& types() const{
                return td;
            }
            TypeDatabase& types(){
                return td;
            }
            FunctionDatabase& functions(){
                return fd;
            }
            const FunctionDatabase& functions() const{
                return fd;
            }
    };
}
namespace bbe{
    BBE_EXPORT ProjectEntitiesPool;
}
