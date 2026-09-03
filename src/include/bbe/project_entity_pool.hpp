#pragma once
#include"function.hpp"
#include"type.hpp"
namespace bbe::impl{
    struct SCM{
        FunctionDatabase::consolidation_map fcmap;
    };
    class ProjectEntitiesPool{
        TypeDatabase td;
        FunctionDatabase fd;
        public:
            ProjectEntitiesPool() = default;
            ProjectEntitiesPool(cppp::frozen_byte_view& b) : td(b), fd(b,td){}
            void garbage_collect(){
                LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper swp{td.sweep()};
                fd.trace_types(swp);
            }
            SCM serialize(cppp::bytes& dst) const{
                SCM scm{.fcmap{fd.make_consolidation_map()}};
                td.serialize(dst);
                fd.serialize(dst,scm.fcmap);
                return scm;
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
    BBE_EXPORT SCM;
    BBE_EXPORT ProjectEntitiesPool;
}
