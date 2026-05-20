#pragma once
#include"commons.hpp"
#include"entity_pool.hpp"
#include<cppp/object-view.hpp>
#include<cppp/assert.hpp>
#include<cppp/array.hpp>
#include<cppp/int.hpp>
#include<unordered_map>
#include<functional>
#include<algorithm>
#include<execution>
#include<numeric>
#include<bit>
namespace bbe::impl{
    using type_id = std::uint32_t;
    enum class TypeCategory : std::uint8_t{
        VOID,SIGNED_INTEGRAL,UNSIGNED_INTEGRAL,PACK,FUNCTION_POINTER
    };
    class type_pack;
    class FunctionSignature;
    class TypeInfo : public Entity<type_id>{
        std::uint64_t _size;
        std::uint64_t align;
        const void* data;
        TypeCategory _type;
        friend class TypeDatabase;
        public:
            TypeInfo(type_id id,TypeCategory t,std::uint64_t sz,std::uint64_t al) : Entity(id), _size(sz), align(al), data(nullptr), _type(t){}
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
    class type_pack{
        cppp::fixed_array<const TypeInfo*> arr;
        using view_t = cppp::view<const TypeInfo*>;
        public:
            type_pack(cppp::fixed_array<const TypeInfo*>&& a) : arr(a){}
            const cppp::fixed_array<const TypeInfo*>& types() const{
                return arr;
            }
            bool operator==(const type_pack& other) const{
                return std::ranges::equal(arr,other.arr);
            }
    };
    struct type_hash{
        private:
            static std::size_t mix_shift(const TypeInfo* v){
                return std::rotl(static_cast<std::size_t>(v->index()),static_cast<std::uint16_t>(7*reinterpret_cast<std::uintptr_t>(v)));
            }
        public:
            static std::size_t operator()(const type_pack& tp){
                return std::transform_reduce(std::execution::unseq,tp.types().begin(),tp.types().end(),0uz,std::bit_xor<std::size_t>{},mix_shift);
            }
    };
    class FunctionSignature{
        const TypeInfo* ret;
        const TypeInfo* par;
        public:
            FunctionSignature(const TypeInfo* r,const TypeInfo* a) : ret(r), par(a){}
            const TypeInfo* return_type() const{
                return ret;
            }
            const TypeInfo* parameter() const{
                return par;
            }
            bool operator==(const FunctionSignature& other) const{
                return ret == other.ret && par == other.par;
            }
    };
    struct fsig_hash{
        static std::size_t operator()(FunctionSignature fs){
            return fs.return_type()->index() ^ std::rotl(fs.parameter()->index(),7);
        }
    };
    class TypeDatabase{
        mutable EntityPool<TypeInfo> infos;
        mutable std::unordered_map<type_pack,const TypeInfo*,type_hash> packs;
        mutable std::unordered_map<FunctionSignature,const TypeInfo*,fsig_hash> functions;
        public:
            constexpr static type_id T_VOID = 0;
            constexpr static type_id T_UINT32 = 1;
            constexpr static type_id T_INT32 = 2;
            constexpr static type_id T_UINT64 = 3;
            constexpr static type_id T_INT64 = 4;
            constexpr static type_id T_BOOL = 5;
            constexpr static type_id T_ERROR = std::numeric_limits<type_id>::max();
            TypeDatabase(){
                using namespace cppp::literals;
                emplace(TypeCategory::VOID,0_u64,0_u64);
                emplace(TypeCategory::UNSIGNED_INTEGRAL,4_u64,4_u64);
                emplace(TypeCategory::SIGNED_INTEGRAL,4_u64,4_u64);
                emplace(TypeCategory::UNSIGNED_INTEGRAL,8_u64,8_u64);
                emplace(TypeCategory::SIGNED_INTEGRAL,8_u64,8_u64);
                emplace(TypeCategory::SIGNED_INTEGRAL,1_u64,1_u64);
            }
            const TypeInfo* pack_of(cppp::fixed_array<const TypeInfo*>&&) const;
            const TypeInfo* function_of(FunctionSignature sig) const;
            template<typename ...A>
            const TypeInfo* emplace(A&& ...a){
                return &infos.emplace(std::forward<A>(a)...);
            }
            const TypeInfo* operator[](type_id i) const{
                CPPP_ASSERT(i != T_ERROR);
                return &infos[i];
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
