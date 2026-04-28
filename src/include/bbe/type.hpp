#pragma once
#include"commons.hpp"
#include"entity_pool.hpp"
#include<cppp/object-view.hpp>
#include<cppp/assert.hpp>
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
            const cppp::fixed_array<type_id>& types() const{
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
                return std::rotr(std::transform_reduce(std::execution::unseq,tp.types().begin(),tp.types().end(),0uz,std::bit_xor<std::size_t>{},mix_shift),static_cast<std::uint16_t>(7*reinterpret_cast<std::uintptr_t>(tp.types().data())));
            }
    };
    class FunctionSignature{
        type_id ret;
        type_id par;
        public:
            FunctionSignature(type_id r,type_id a) : ret(r), par(a){}
            type_id return_type() const{
                return ret;
            }
            type_id parameter() const{
                return par;
            }
            bool operator==(const FunctionSignature& other) const{
                return ret == other.ret && par == other.par;
            }
    };
    struct fsig_hash{
        static std::size_t operator()(FunctionSignature fs){
            return fs.return_type() ^ std::rotl(fs.parameter(),7);
        }
    };
    enum class TypeCategory : std::uint8_t{
        VOID,SIGNED_INTEGRAL,UNSIGNED_INTEGRAL,PACK,FUNCTION_POINTER
    };
    class TypeInfo{
        std::uint64_t _size;
        std::uint64_t align;
        const void* data;
        TypeCategory _type;
        friend class TypeDatabase;
        public:
            TypeInfo(TypeCategory t,std::uint64_t sz,std::uint64_t al) : _size(sz), align(al), data(nullptr), _type(t){}
            std::uint64_t size() const{
                return _size;
            }
            std::uint64_t alignment() const{
                return align;
            }
            std::uint64_t stride() const{
                return _size + (-_size & (align-1));
            }
            TypeCategory type() const{
                return _type;
            }
            const type_pack& pack_contents() const{
                return *static_cast<const type_pack*>(data);
            }
            const FunctionSignature& function_signature() const{
                return *static_cast<const FunctionSignature*>(data);
            }
    };
    class TypeDatabase{
        mutable EntityPool<TypeInfo,type_id> infos;
        mutable std::unordered_map<type_pack,type_id,type_hash> packs;
        mutable std::unordered_map<FunctionSignature,type_id,fsig_hash> functions;
        public:
            constexpr static type_id T_VOID = 0;
            constexpr static type_id T_UINT32 = 1;
            constexpr static type_id T_INT32 = 2;
            constexpr static type_id T_UINT64 = 3;
            constexpr static type_id T_INT64 = 4;
            constexpr static type_id T_BOOL = 5;
            constexpr static type_id T_ERROR = std::numeric_limits<type_id>::max();
            TypeDatabase(){
                emplace(TypeCategory::VOID,0,0);
                emplace(TypeCategory::UNSIGNED_INTEGRAL,4,4);
                emplace(TypeCategory::SIGNED_INTEGRAL,4,4);
                emplace(TypeCategory::UNSIGNED_INTEGRAL,8,8);
                emplace(TypeCategory::SIGNED_INTEGRAL,8,8);
                emplace(TypeCategory::SIGNED_INTEGRAL,1,1);
            }
            type_id pack_of(cppp::fixed_array<type_id>&&) const;
            type_id function_of(FunctionSignature sig) const;
            template<typename ...A>
            type_id emplace(A&& ...a){
                return infos.emplace(std::forward<A>(a)...);
            }
            const TypeInfo& operator[](type_id i) const{
                CPPP_ASSERT(i != T_ERROR);
                return infos[i];
            }
    };
}
namespace bbe{
    BBE_EXPORT type_id;
    BBE_EXPORT type_pack;
    BBE_EXPORT TypeCategory;
    BBE_EXPORT TypeInfo;
    BBE_EXPORT TypeDatabase;
}
