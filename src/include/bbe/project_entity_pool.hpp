#pragma once
#include"function.hpp"
#include"type.hpp"
namespace bbe::impl{
    struct SCM{
        TypeDatabase::consolidation_map tcmap;
        FunctionDatabase::consolidation_map fcmap;
    };
    class ProjectEntitiesPool{
        TypeDatabase td;
        FunctionDatabase fd;
        public:
            ProjectEntitiesPool() = default;
            ProjectEntitiesPool(cppp::frozen_byte_view& b) : td(b), fd(b,td){}
            SCM serialize(cppp::bytes& dst) const{
                SCM scm{.tcmap{td.make_consolidation_map()},.fcmap{fd.make_consolidation_map()}};
                td.serialize(dst,scm.tcmap);
                fd.serialize(dst,scm.tcmap,scm.fcmap);
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
