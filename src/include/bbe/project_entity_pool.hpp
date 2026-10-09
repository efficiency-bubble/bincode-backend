#pragma once
#include"function.hpp"
#include"overload.hpp"
#include"type-database.hpp"
namespace bbe::impl{
    class ProjectEntitiesPool{
        TypeDatabase td;
        EntityPool<Function> fd;
        EntityPool<OverloadSet> od;
        static EntityPool<Function> _deser_f(cppp::frozen_byte_view& b,const TypeDatabase& td){
            EntityPool<Function> fd{b};
            for(func_id i=0;i<fd.size();++i){
                fd[i].deserialize(b,td);
            }
            return fd;
        }
        static EntityPool<OverloadSet> _deser_o(cppp::frozen_byte_view& b,const EntityPool<Function>& fd){
            EntityPool<OverloadSet> od{b};
            for(func_id i=0;i<od.size();++i){
                od[i].deserialize(b,fd);
            }
            return od;
        }
        public:
            ProjectEntitiesPool() = default;
            ProjectEntitiesPool(cppp::frozen_byte_view& b) : td(b), fd(_deser_f(b,td)), od(_deser_o(b,fd)){}
            EntitySweeper begin_gc(){
                return {td.sweep(),fd.sweep(),od.sweep()};
            }
            void end_gc(EntitySweeper swp){
                td.finalize_gc(swp);
            }
            void serialize(cppp::bytes& dst) const{
                td.serialize(dst);
                fd.serialize(dst);
                od.serialize(dst);
            }
            const TypeDatabase& types() const{
                return td;
            }
            TypeDatabase& types(){
                return td;
            }
            EntityPool<Function>& functions(){
                return fd;
            }
            const EntityPool<Function>& functions() const{
                return fd;
            }
            EntityPool<OverloadSet>& overloads(){
                return od;
            }
            const EntityPool<OverloadSet>& overloads() const{
                return od;
            }
    };
}
namespace bbe{
    BBE_EXPORT ProjectEntitiesPool;
}
