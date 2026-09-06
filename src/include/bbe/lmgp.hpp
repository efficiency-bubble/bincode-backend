#pragma once
#include"commons.hpp"
#include<cppp/memory.hpp>
#include<cppp/int.hpp>
#include<utility>
#include<memory>
#include<cmath>
#include<array>
#include<bit>
namespace bbe::impl{
    template<typename I>
    class Entity{
        mutable I _index;
        template<typename E,E::id_type>
        friend class LinearMovingGarbageCollectedPool;
        // GC marking isn't *really* a mutating operation, is it?
        void mark(I other) const{
            _index = static_cast<I>((other << 1) | 1);
        }
        void unmark() const{
            _index &= ~static_cast<I>(1);
        }
        public:
            bool marked() const{
                return (_index & 1) != 0;
            }
            Entity(I k) : _index(k << 1){}
            Entity(const Entity&) = delete;
            Entity(Entity&&) = default;
            Entity& operator=(const Entity&) = delete;
            Entity& operator=(Entity&&) = default;
            using id_type = I;
            I index() const{
                return _index >> 1;
            }
    };
    template<typename E,E::id_type n = cppp::safe_cast<typename E::id_type>(std::max(std::bit_ceil(4096uz / sizeof(E)),16uz))>
    class LinearMovingGarbageCollectedPool{
        using id_type = E::id_type;
        E** storage;
        id_type length;
        id_type full_block_count() const{
            return length / n;
        }
        id_type block_capacity() const{
            return std::bit_ceil((length + n - 1) / n);
        }
        template<bool _const>
        class _iterator{
            using pool_type = std::conditional_t<_const,const LinearMovingGarbageCollectedPool,LinearMovingGarbageCollectedPool>;
            pool_type* storage;
            id_type block;
            id_type item;
            friend LinearMovingGarbageCollectedPool;
            _iterator(pool_type& p,id_type b,id_type i) : storage(&p), block(b), item(i){}
            public:
                constexpr _iterator() noexcept : storage(nullptr), block(0), item(0){}
                using value_type = std::conditional_t<_const,const E,E>;
                value_type& operator*() const{
                    return storage->storage[block][item];
                }
                value_type* operator->() const{
                    return &storage->storage[block][item];
                }
                bool operator==(const _iterator& other) const{
                    return block == other.block && item == other.item;
                }
                _iterator& operator++(){
                    if(++item == n){
                        item = 0;
                        ++block;
                    }
                    return *this;
                }
                _iterator& operator++(int){
                    _iterator dup{*this};
                    ++*this;
                    return dup;
                }
                _iterator& operator--(){
                    if(!item--){
                        item += n;
                        --block;
                    }
                    return *this;
                }
                _iterator& operator--(int){
                    _iterator dup{*this};
                    --*this;
                    return dup;
                }
        };
        void destroy_block(id_type i){
            std::destroy_n(storage[i],n);
            std::allocator<E>().deallocate(storage[i],n);
            
            // Only for the peace of mind. Pointers are trivially-destructible so this is a no-op.
            std::destroy_at(storage + i);
        }
        // ilast is the last block index, but it needs only be correct if length % n != 0, so simply always using full_block_count() + 1 is fine. It is an argument because it can probably be stolen from nearby iteration code, to save having to recalculate it.
        void destroy_last_block(id_type ilast){
            if(id_type last_block_population = length % n){
                std::destroy_n(storage[ilast],last_block_population);
                std::allocator<E>().deallocate(storage[ilast],last_block_population);
                std::destroy_at(storage + ilast);
            }
        }
        void destroy(){
            if(storage){
                id_type i;
                for(i = 0;i < full_block_count();++i){
                    destroy_block(i);
                }
                destroy_last_block(i);
                std::allocator<E*>().deallocate(storage,block_capacity());
            }
        }
        public:
            LinearMovingGarbageCollectedPool() : storage(nullptr), length(0uz){}
            LinearMovingGarbageCollectedPool(const LinearMovingGarbageCollectedPool&) = delete;
            LinearMovingGarbageCollectedPool(LinearMovingGarbageCollectedPool&& other) : storage(std::exchange(other.storage,nullptr)), length(other.length){}
            LinearMovingGarbageCollectedPool& operator=(const LinearMovingGarbageCollectedPool&) = delete;
            LinearMovingGarbageCollectedPool& operator=(LinearMovingGarbageCollectedPool&& other){
                if(this != &other){
                    destroy();
                    storage = std::exchange(other.storage,nullptr);
                    length = other.length;
                }
                return *this;
            }
            class Sweeper{
                LinearMovingGarbageCollectedPool* pool;
                id_type counter;
                friend LinearMovingGarbageCollectedPool;
                Sweeper(LinearMovingGarbageCollectedPool& gcp) : pool(&gcp), counter(0){
                    for(auto& el : gcp){
                        el.unmark();
                    }
                }
                public:
                    const E& query(id_type oldid) const{
                        return (*pool)[oldid];
                    }
                    void trace(id_type& id){
                        E& entity = (*pool)[id];
                        if(!entity.marked()){
                            entity.mark(counter++);
                        }
                        id = entity.index();
                    }
                    void trace(E*& ptr){
                        if(!ptr->marked()){
                            ptr->mark(counter++);
                        }
                        ptr = &(*pool)[ptr->index()];
                    }
                    void trace(const E*& ptr){
                        if(!ptr->marked()){
                            ptr->mark(counter++);
                        }
                        ptr = &(*pool)[ptr->index()];
                    }
                    ~Sweeper(){
                        id_type i = 0;
                        for(auto& el : *pool){
                            while(el.marked() && i != el.index()){
                                std::ranges::swap(el,(*pool)[el.index()]);
                            }
                            ++i;
                        }
                        pool->erase_after(counter);
                    }
            };
            template<typename ...A>
            E& emplace(A&& ...a){
                id_type i = length;
                E* el;
                if(id_type pop = length % n){
                    el = new(storage[length / n] + pop) E(i,std::forward<A>(a)...);
                }else{
                    cppp::uninitialized_memory<E> new_block{n};
                    el = &new_block.emplace_at(0uz,i,std::forward<A>(a)...);
                    if(id_type block_count = length / n){
                        if(std::has_single_bit(block_count)){
                            try{
                                cppp::grow_and_emplace_back(storage,block_count,block_count*2,new_block.get());
                            }catch(...){
                                new_block.destroy_at(0uz);
                                throw;
                            }
                        }else new(storage + block_count) E*(new_block.get());
                    }else{
                        storage = std::allocator<E*>().allocate(1);
                        new(storage) E*(new_block.get());
                    }
                    new_block.release();
                }
                ++length;
                return *el;
            }
            Sweeper sweep(){
                return {*this};
            }
            void erase_after(id_type i){
                id_type old_capacity = block_capacity();
                id_type block = i / n;
                if(id_type start_index = i % n){
                    std::ranges::destroy(storage[block] + start_index,storage[block] + n);
                    ++block;
                }
                for(const id_type fbc = full_block_count();block < fbc;++block){
                    destroy_block(block);
                }
                destroy_last_block(block);
                length = i;
                if(block_capacity() < old_capacity){
                    storage = cppp::shrink(storage,(length + n - 1) / n,old_capacity,block_capacity());
                }
            }
            id_type size() const{
                return length;
            }
            E& operator[](id_type i){
                return storage[i / n][i % n];
            }
            const E& operator[](id_type i) const{
                return storage[i / n][i % n];
            }
            using iterator = _iterator<false>;
            using const_iterator = _iterator<true>;
            iterator begin(){
                return {*this,0,0};
            }
            const_iterator begin() const{
                return {*this,0,0};
            }
            iterator end(){
                return {*this,length / n,length % n};
            }
            const_iterator end() const{
                return {*this,length / n,length % n};
            }
            ~LinearMovingGarbageCollectedPool(){
                destroy();
            }
    };
}
namespace bbe{
    BBE_EXPORT LinearMovingGarbageCollectedPool;
}
