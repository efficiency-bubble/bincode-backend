#pragma once
#include"lmgp.hpp"
namespace bbe::impl{
    template<typename T>
    class EntityPool{
        public:
            using id_type = T::id_type;
        private:
            using container_type = LinearMovingGarbageCollectedPool<T>;
            container_type obj;
        public:
            EntityPool() = default;
            id_type size() const{
                return obj.size();
            }
            EntityPool(id_type from,cppp::frozen_byte_view& buf){
                id_type n = from + cppp::muleb128_r<id_type>(buf);
                for(id_type i=0;i<n;++i){
                    obj.emplace(uninitialize);
                }
            }
            EntityPool(cppp::frozen_byte_view& buf) : EntityPool(static_cast<id_type>(0),buf){}
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w<id_type>(dst,size());
                for(const auto& ent : *this){
                    ent.serialize(dst);
                }
            }
            container_type::Sweeper sweep(){
                return obj.sweep();
            }
            template<typename ...A>
            T& emplace(A&& ...a){
                return obj.emplace(std::forward<A>(a)...);
            }
            const T& operator[](id_type key) const{
                return obj[key];
            }
            T& operator[](id_type key){
                return obj[key];
            }
            using iterator = container_type::iterator;
            using const_iterator = container_type::const_iterator;
            iterator begin(){
                return obj.begin();
            }
            iterator end(){
                return obj.end();
            }
            const_iterator begin() const{
                return obj.begin();
            }
            const_iterator end() const{
                return obj.end();
            }
    };
}
namespace bbe{
    BBE_EXPORT EntityPool;
}
