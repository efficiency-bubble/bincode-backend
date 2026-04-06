#pragma once
#include"commons.hpp"
#include"entity_pool.hpp"
#include<cppp/object-view.hpp>
#include<cppp/array.hpp>
#include<unordered_map>
#include<functional>
#include<algorithm>
#include<execution>
#include<numeric>
#include<bit>
namespace bbe::impl{
    using type_id = std::uint32_t;
    class type_pack{
        cppp::fixed_array<type_id> arr;
        using view_t = cppp::view<type_id>;
        public:
            type_pack(cppp::fixed_array<type_id>&& a) : arr(a){}
            const cppp::fixed_array<type_id>& array() const{
                return arr;
            }
            bool operator==(const type_pack& other) const{
                return std::ranges::equal(arr,other.arr);
            }
    };
    struct type_hash{
        private:
            static std::size_t mix_shift(const type_id& v){
                return std::rotl(static_cast<std::size_t>(v),static_cast<std::uint16_t>(7*reinterpret_cast<std::uintptr_t>(&v)));
            }
        public:
            static std::size_t operator()(const type_pack& tp){
                return std::rotr(std::transform_reduce(std::execution::unseq,tp.array().begin(),tp.array().end(),0uz,std::bit_xor<std::size_t>{},mix_shift),static_cast<std::uint16_t>(7*reinterpret_cast<std::uintptr_t>(tp.array().data())));
            }
    };
    class TypeInfo{
        std::uint64_t _size;
        std::uint64_t _align;
        const type_pack* _contents;
        friend class TypeDatabase;
        public:
            TypeInfo(std::uint64_t sz,std::uint64_t al) : _size(sz), _align(al), _contents(nullptr){}
            std::uint64_t size() const{
                return _size;
            }
            std::uint64_t alignment() const{
                return _align;
            }
            const type_pack& pack_contents() const{
                return *_contents;
            }
    };
    
    class TypeDatabase{
        EntityPool<TypeInfo,type_id> infos;
        std::unordered_map<type_pack,type_id,type_hash> packs;
        public:
            constexpr static type_id T_VOID = 0;
            constexpr static type_id T_UINT32 = 1;
            constexpr static type_id T_UINT64 = 2;
            constexpr static type_id T_BOOL = 3;
            constexpr static type_id T_ERROR = std::numeric_limits<type_id>::max();
            TypeDatabase(){
                emplace(0,0);
                emplace(4,4);
                emplace(8,8);
                emplace(1,1);
            }
            type_id pack_of(cppp::fixed_array<type_id>&& a){
                type_pack key{std::move(a)};
                if(auto it=packs.find(key);it!=packs.end()){
                    return it->second;
                }else{
                    std::uint64_t size = 0, align = 0;
                    for(const type_id i : key.array()){
                        size += infos[i].size();
                        align = std::max(align,infos[i].alignment());
                    }
                    type_id nt = emplace(size,align);
                    infos[nt]._contents = &packs.try_emplace(std::move(key),nt).first->first;
                    return nt;
                }
            }
            template<typename ...A>
            type_id emplace(A&& ...a){
                return infos.emplace(std::forward<A>(a)...);
            }
            const TypeInfo& operator[](type_id i) const{
                return infos[i];
            }
    };
}
namespace bbe{
    BBE_EXPORT TypeInfo;
    BBE_EXPORT TypeDatabase;
}
