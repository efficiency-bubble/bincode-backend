#pragma once
#include"commons.hpp"
#include"serialization.hpp"
#include"uninit.hpp"
#include"entity_pool.hpp"
#include"idfwd.hpp"
#include<cppp/object-view.hpp>
#include<cppp/assert.hpp>
#include<cppp/array.hpp>
#include<cppp/int.hpp>
#include<unordered_map>
#include<unordered_set>
#include<functional>
#include<algorithm>
#include<execution>
#include<numeric>
#include<bit>
namespace bbe::impl{
    enum class TypeCategory : std::uint8_t{
        VOID,SIGNED_INTEGRAL,UNSIGNED_INTEGRAL,PACK,FUNCTION_POINTER
    };
    class type_pack;
    class FunctionSignature;
    class TypeDatabase;
    class TypeInfo : public Entity<type_id>{
        std::uint64_t _size;
        std::uint64_t align;
        const void* data;
        TypeCategory _type;
        friend TypeDatabase;
        public:
            TypeInfo(type_id id,TypeCategory t,std::uint64_t sz,std::uint64_t al) : Entity(id), _size(sz), align(al), data(nullptr), _type(t){}
            TypeInfo(type_id id,uninitialize_t) : Entity(id){}
            void initialize(TypeCategory t,std::uint64_t sz,std::uint64_t al){
                _size = sz;
                align = al;
                data = nullptr;
                _type = t;
            }
            inline void serialize(cppp::bytes& dst) const;
            inline void deserialize(cppp::frozen_byte_view& buf,TypeDatabase&);
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
    static_assert(std::move_constructible<TypeInfo>);
    inline type_id optindex(const TypeInfo* p){
        return p?p->index():std::numeric_limits<type_id>::max();
    }
    class type_pack{
        cppp::fixed_array<const TypeInfo*> arr;
        using view_t = cppp::view<const TypeInfo*>;
        public:
            type_pack(cppp::fixed_array<const TypeInfo*>&& a) : arr(a){}
            inline type_pack(cppp::frozen_byte_view&,const TypeDatabase&);
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w<std::uint64_t>(dst,arr.size());
                for(const TypeInfo* p : arr){
                    cppp::muleb128_w<type_id>(dst,p->index());
                }
            }
            void trace_types(LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper& swp){
                for(auto& t : arr){
                    swp.trace(t);
                }
            }
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
            FunctionSignature(uninitialize_t){}
            FunctionSignature(const TypeInfo* r,const TypeInfo* a) : ret(r), par(a){}
            FunctionSignature(cppp::frozen_byte_view& buf,const TypeDatabase& tdb) : FunctionSignature(uninitialize){
                deserialize(buf,tdb);
            }
            inline void deserialize(cppp::frozen_byte_view&,const TypeDatabase&);
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w<type_id>(dst,ret->index());
                cppp::muleb128_w<type_id>(dst,par->index());
            }
            void trace_types(LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper& swp){
                swp.trace(ret);
                swp.trace(par);
            }
            void set_return(const TypeInfo* t){
                ret = t;
            }
            const TypeInfo* return_type() const{
                return ret;
            }
            void set_param(const TypeInfo* t){
                par = t;
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
            return optindex(fs.return_type()) ^ std::rotl(optindex(fs.parameter()),7);
        }
    };
    class TypeDatabase{
        mutable EntityPool<TypeInfo> infos;
        mutable std::unordered_map<type_pack,const TypeInfo*,type_hash> packs;
        mutable std::unordered_map<FunctionSignature,const TypeInfo*,fsig_hash> functions;
        
        friend TypeInfo;
        // these are non-const even though `packs` and `functions` are mutable, since they touch the cache manually
        // and thus is able to change visible behavior (namely, break it)
        const type_pack& inject_pack(type_pack&& pack,const TypeInfo& inf){
            return packs.try_emplace(std::move(pack),&inf).first->first;
        }
        const FunctionSignature& inject_sig(FunctionSignature sig,const TypeInfo& inf){
            return functions.try_emplace(sig,&inf).first->first;
        }
        constexpr static type_id T_INTRINSIC_END = 6;
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
            TypeDatabase(cppp::frozen_byte_view& buf) : infos(T_INTRINSIC_END,buf){
                using namespace cppp::literals;
                infos[T_VOID].initialize(TypeCategory::VOID,0_u64,0_u64);
                infos[T_UINT32].initialize(TypeCategory::UNSIGNED_INTEGRAL,4_u64,4_u64);
                infos[T_INT32].initialize(TypeCategory::SIGNED_INTEGRAL,4_u64,4_u64);
                infos[T_UINT64].initialize(TypeCategory::UNSIGNED_INTEGRAL,8_u64,8_u64);
                infos[T_INT64].initialize(TypeCategory::SIGNED_INTEGRAL,8_u64,8_u64);
                infos[T_BOOL].initialize(TypeCategory::SIGNED_INTEGRAL,1_u64,1_u64);
                for(type_id i=T_INTRINSIC_END;i<infos.size();++i){
                    infos[i].deserialize(buf,*this);
                }
            }
            LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper sweep(){
                LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper swp{infos.sweep()};
                for(type_id i=0;i<T_INTRINSIC_END;++i){
                    swp.trace(i);
                }
                return swp;
            }
            void rebuild_caches() const{
                // TODO: make this more efficient
                std::unordered_set<const type_pack*> unused_packs;
                std::unordered_set<const FunctionSignature*> unused_fns;
                for(const auto& pk : packs | std::views::keys){
                    unused_packs.emplace(&pk);
                }
                for(const auto& fn : functions | std::views::keys){
                    unused_fns.emplace(&fn);
                }
                for(const auto& t : infos){
                    switch(t.type()){
                        case TypeCategory::PACK:
                            unused_packs.erase(&t.pack_contents());
                            break;
                        case TypeCategory::FUNCTION_POINTER:
                            unused_fns.erase(&t.function_signature());
                            break;
                        default:;
                    }
                }
                for(const type_pack* up : unused_packs){
                    packs.erase(*up);
                }
                for(const FunctionSignature* uf : unused_fns){
                    functions.erase(*uf);
                }
            }
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w<type_id>(dst,infos.size() - T_INTRINSIC_END);
                for(const TypeInfo& ent : infos){
                    if(ent.index() >= T_INTRINSIC_END){
                        ent.serialize(dst);
                    }
                }
            }
            std::size_t size() const{
                return infos.size();
            }
            const TypeInfo& pack_of(cppp::fixed_array<const TypeInfo*>&&) const;
            const TypeInfo& function_of(FunctionSignature sig) const;
            template<typename ...A>
            const TypeInfo& emplace(A&& ...a){
                return infos.emplace(std::forward<A>(a)...);
            }
            const TypeInfo& operator[](type_id i) const{
                CPPP_ASSERT(i != T_ERROR);
                return infos[i];
            }
            const TypeInfo* getopt(type_id i) const{
                if(i == T_ERROR) return nullptr;
                return &infos[i];
            }
    };
    inline type_pack::type_pack(cppp::frozen_byte_view& buf,const TypeDatabase& tdb) : arr(cppp::muleb128_r<std::uint64_t>(buf)){
        for(const TypeInfo*& p : arr){
            p = &tdb[cppp::muleb128_r<type_id>(buf)];
        }
    }
    inline void FunctionSignature::deserialize(cppp::frozen_byte_view& buf,const TypeDatabase& tdb){
        ret = &tdb[cppp::muleb128_r<type_id>(buf)];
        par = &tdb[cppp::muleb128_r<type_id>(buf)];
    }
    inline void TypeInfo::serialize(cppp::bytes& dst) const{
        cppp::muleb128_w<std::uint64_t>(dst,_size);
        cppp::muleb128_w<std::uint64_t>(dst,align);
        dst.appendl(static_cast<std::uint8_t>(_type));
        switch(_type){
            case TypeCategory::PACK:
                pack_contents().serialize(dst);
                break;
            case TypeCategory::FUNCTION_POINTER:
                function_signature().serialize(dst);
                break;
            default:;
        }
    }
    inline void TypeInfo::deserialize(cppp::frozen_byte_view& buf,TypeDatabase& tdb){
        _size = cppp::muleb128_r<std::uint64_t>(buf);
        align = cppp::muleb128_r<std::uint64_t>(buf);
        switch(_type = static_cast<TypeCategory>(cppp::read<std::uint8_t>(buf))){
            case TypeCategory::PACK:
                data = &tdb.inject_pack({buf,tdb},*this);
                break;
            case TypeCategory::FUNCTION_POINTER:
                data = &tdb.inject_sig({buf,tdb},*this);
                break;
            default:;
        }
    }
}
namespace bbe{
    BBE_EXPORT type_id;
    BBE_EXPORT type_pack;
    BBE_EXPORT TypeCategory;
    BBE_EXPORT TypeInfo;
    BBE_EXPORT TypeDatabase;
}
