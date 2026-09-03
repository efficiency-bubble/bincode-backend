#pragma once
#include"commons.hpp"
#include"serialization.hpp"
#include"uninit.hpp"
#include<cppp/freelist.hpp>
#include<unordered_set>
#include<unordered_map>
#include<concepts>
#include<iterator>
namespace bbe::impl{
    template<typename I>
    class HashedEntity{
        // const because changing this will break the hash
        const I _index;
        public:
            HashedEntity(I k) : _index(k){}
            // probably shouldn't let this be copyable or movable, considering we expect pointers to us
            HashedEntity(const HashedEntity&) = delete;
            HashedEntity(HashedEntity&&) = delete;
            using id_type = I;
            I index() const{
                return _index;
            }
    };
    template<typename T>
    struct entity_wrapper{
        mutable T e;
        template<typename ...A>
        entity_wrapper(A&& ...a) : e(std::forward<A>(a)...){}
    };
    template<typename I>
    struct id_hash{
        using is_transparent = void;
        constexpr static std::size_t operator()(typename I::id_type i){
            return std::hash<typename I::id_type>{}(i);
        }
        constexpr static std::size_t operator()(const entity_wrapper<I>& e){
            return std::hash<typename I::id_type>{}(e.e.index());
        }
    };
    template<typename I>
    struct id_eq{
        using is_transparent = void;
        constexpr static bool operator()(const entity_wrapper<I>& e,const entity_wrapper<I>& f){
            return e.e.index() == f.e.index();
        }
        constexpr static bool operator()(typename I::id_type i,const entity_wrapper<I>& e){
            return i == e.e.index();
        }
        constexpr static bool operator()(const entity_wrapper<I>& e,typename I::id_type i){
            return e.e.index() == i;
        }
    };
    template<typename T>
    class HashedEntityPool{
        public:
            using id_type = T::id_type;
            using consolidation_map = std::unordered_map<id_type,id_type>;
        private:
            using container_type = std::unordered_set<entity_wrapper<T>,id_hash<T>,id_eq<T>>;
            container_type obj;
            cppp::freelist<id_type> fl;
            template<bool is_const>
            class _iterator{
                friend HashedEntityPool<T>;
                container_type::const_iterator underlying;
                _iterator(container_type::const_iterator it) : underlying(it){}
                public:
                    using value_type = std::conditional_t<is_const,const T,T>;
                    using difference_type = std::ptrdiff_t;
                    using pointer = value_type*;
                    using reference = value_type&;
                    using iterator_category = std::forward_iterator_tag;
                    _iterator() = default;
                    reference operator*() const{
                        return underlying->e;
                    }
                    pointer operator->() const{
                        return &underlying->e;
                    }
                    _iterator& operator++(){
                        ++underlying;
                        return *this;
                    }
                    _iterator operator++(int){
                        return ++_iterator(*this);
                    }
                    bool operator==(_iterator other) const{
                        return underlying == other.underlying;
                    }
            };
        public:
            HashedEntityPool() = default;
            id_type size() const{
                return static_cast<id_type>(obj.size());
            }
            template<typename ...Ctx>
            HashedEntityPool(id_type from,cppp::frozen_byte_view& buf,Ctx& ...ctx) : fl(from+cppp::muleb128_r<id_type>(buf)){
                for(id_type i=from;i<fl.size();++i){
                    obj.emplace(i,uninitialize);
                }
            }
            template<typename ...Ctx>
            HashedEntityPool(cppp::frozen_byte_view& buf,Ctx& ...ctx) : HashedEntityPool(static_cast<id_type>(0),buf,ctx...){}
            consolidation_map make_consolidation_map() const{
                consolidation_map mp;
                id_type index = 0;
                for(const auto& ent : *this){
                    mp.try_emplace(ent.index(),index++);
                }
                return mp;
            }
            template<typename ...Ctx>
            void serialize(cppp::bytes& dst,Ctx& ...ctx) const{
                cppp::muleb128_w<id_type>(dst,size());
                for(const auto& ent : *this){
                    ent.serialize(dst,ctx...);
                }
            }
            template<typename ...A>
            T& emplace(A&& ...a){
                return obj.emplace(fl.allocate(),std::forward<A>(a)...).first->e;
            }
            template<typename ...A  >
            T& emplace_at(id_type at,A&& ...a){
                return obj.emplace(at,std::forward<A>(a)...).first->e;
            }
            void pop(id_type key){
                fl.deallocate(key);
                obj.erase(key);
            }
            bool occupied(id_type key) const{
                return fl.occupied(key);
            }
            const T& operator[](id_type key) const{
                return obj.find(key)->e;
            }
            T& operator[](id_type key){
                return obj.find(key)->e;
            }
            using iterator = _iterator<false>;
            using const_iterator = _iterator<true>;
            iterator begin(){
                return obj.cbegin();
            }
            iterator end(){
                return obj.cend();
            }
            const_iterator begin() const{
                return obj.cbegin();
            }
            const_iterator end() const{
                return obj.cend();
            }
    };
}
namespace bbe{
    BBE_EXPORT HashedEntityPool;
}
