#pragma once
#include"function.hpp"
#include"type.hpp"
namespace bbe::impl{
    class ProjectEntitiesPool{
        TypeDatabase td;
        FunctionDatabase fd;
        public:
            ProjectEntitiesPool() = default;
            ProjectEntitiesPool(cppp::frozen_byte_view& b) : td(b), fd(b,td){}
            void serialize(cppp::bytes& dst) const{
                const TypeDatabase::consolidation_map tcmap{td.make_consolidation_map()};
                td.serialize(dst,tcmap);
                const FunctionDatabase::consolidation_map fcmap{fd.make_consolidation_map()};
                fd.serialize(dst,tcmap,fcmap);
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
