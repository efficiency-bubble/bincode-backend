#pragma once
#include"serialization.hpp"
#include"entity_pool.hpp"
#include"function.hpp"
#include"idfwd.hpp"
#include"type-database.hpp"
#include<cppp/string.hpp> // compatibility names
#include<vector>
namespace bbe::impl{
    class ProjectEntitiesPool;
    class ErrorDatabase;
    class FunctionDatabase{
        using pool_type = EntityPool<Function>;
        pool_type funcs;
        public:
            FunctionDatabase() = default;
            FunctionDatabase(cppp::frozen_byte_view& buf,const TypeDatabase& tdb) : funcs(buf){
                for(func_id i=0;i<funcs.size();++i){
                    funcs[i].deserialize(buf,tdb);
                }
            }
            LinearMovingGarbageCollectedPool<Function>::Sweeper sweep(){
                return funcs.sweep();
            }
            void serialize(cppp::bytes& dst) const{
                funcs.serialize(dst);
            }
            template<typename ...A>
            Function& emplace(A&& ...a){
                return funcs.emplace(std::forward<A>(a)...);
            }
            std::size_t size() const{
                return funcs.size();
            }
            Function& operator[](func_id i){
                return funcs[i];
            }
            const Function& operator[](func_id i) const{
                return funcs[i];
            }
            using iterator = pool_type::iterator;
            using const_iterator = pool_type::const_iterator;
            iterator begin(){
                return funcs.begin();
            }
            iterator end(){
                return funcs.end();
            }
            const_iterator begin() const{
                return funcs.begin();
            }
            const_iterator end() const{
                return funcs.end();
            }
    };
}
namespace bbe{
    BBE_EXPORT FunctionSignature;
    BBE_EXPORT FunctionDatabase;
}
