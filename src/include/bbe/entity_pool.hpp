#pragma once
#include"function.hpp"
#include<unordered_map>
#include<cppp/freelist.hpp>
namespace bbe::impl{
    template<typename T,typename K=std::uint32_t>
    class EntityPool{
        std::unordered_map<K,T> obj;
        cppp::freelist<K> fl;
        public:
            EntityPool(){}
            template<typename ...A>
            K emplace(A&& ...a){
                K key = fl.allocate();
                obj.try_emplace(key,std::forward<A>(a)...);
                return key;
            }
            void pop(K key){
                fl.deallocate(key);
                obj.erase(key);
            }
            const T& operator[](K key) const{
                return obj.at(key);
            }
            T& operator[](K key){
                return obj.at(key);
            }
            using iterator = std::unordered_map<K,T>::iterator;
            using const_iterator = std::unordered_map<K,T>::const_iterator;
            iterator begin(){
                return obj.begin();
            }
            iterator end(){
                return obj.end();
            }
            const_iterator begin() const{
                return obj.cbegin();
            }
            const_iterator end() const{
                return obj.cend();
            }
    };
    class ProjectEntitiesPool{
        public:
            using index_type = std::uint32_t;
        private:
            EntityPool<Function,index_type> fn_p;
        public:
            EntityPool<Function,index_type>& function_pool(){
                return fn_p;
            }
            const EntityPool<Function,index_type>& function_pool() const{
                return fn_p;
            }
    };
}
namespace bbe{
    BBE_EXPORT EntityPool;
    BBE_EXPORT ProjectEntitiesPool;
}
