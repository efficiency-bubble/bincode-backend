#pragma once
#include"function-database.hpp"
#include"type-database.hpp"
namespace bbe::impl{
    class ProjectEntitiesPool{
        TypeDatabase td;
        FunctionDatabase fd;
        public:
            ProjectEntitiesPool() = default;
            ProjectEntitiesPool(cppp::frozen_byte_view& b) : td(b), fd(b,td){}
            EntitySweeper begin_gc(){
                return {td.sweep(),fd.sweep()};
            }
            void end_gc(EntitySweeper swp){
                td.finalize_gc(swp);
            }
            void serialize(cppp::bytes& dst) const{
                td.serialize(dst);
                fd.serialize(dst);
            }
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
