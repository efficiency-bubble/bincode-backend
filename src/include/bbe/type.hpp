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
        VOID,SIGNED_INTEGRAL,UNSIGNED_INTEGRAL,PACK,FUNCTION_POINTER,POINTER
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
            const TypeInfo& pointee() const{
                return *static_cast<const TypeInfo*>(data);
            }
    };
    inline type_id optindex(const TypeInfo* p){
        return p?p->index():std::numeric_limits<type_id>::max();
    }
    class type_pack{
        cppp::fixed_array<const TypeInfo*> arr;
        using view_t = cppp::view<const TypeInfo*>;
        friend class TypeDatabase;
        public:
            type_pack(cppp::fixed_array<const TypeInfo*>&& a) : arr(a){}
            inline type_pack(cppp::frozen_byte_view&,const TypeDatabase&);
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w<std::uint64_t>(dst,arr.size());
                for(const TypeInfo* p : arr){
                    cppp::muleb128_w<type_id>(dst,p->index());
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
        friend TypeDatabase;
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
        // TODO: get rid of compound type lookup caches
        using packs_t = std::unordered_map<type_pack,const TypeInfo*,type_hash>;
        mutable packs_t packs;
        using funcs_t = std::unordered_map<FunctionSignature,const TypeInfo*,fsig_hash>;
        mutable funcs_t functions;
        using ptrs_t = std::unordered_map<const TypeInfo*,const TypeInfo*>;
        mutable ptrs_t pointers;
        
        friend TypeInfo;
        const type_pack& inject_pack(type_pack&& pack,const TypeInfo& inf) const{
            return packs.try_emplace(std::move(pack),&inf).first->first;
        }
        const FunctionSignature& inject_sig(FunctionSignature sig,const TypeInfo& inf) const{
            return functions.try_emplace(sig,&inf).first->first;
        }
        const TypeInfo& inject_ptr(const TypeInfo& under,const TypeInfo& inf) const{
            return *pointers.try_emplace(&under,&inf).first->first;
        }
        constexpr static type_id T_INTRINSIC_END = 6;
        void trace_type(const TypeInfo*& t,LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper& swp) const{
            swp.trace(t);
            switch(t->type()){
                case TypeCategory::PACK:
                    trace_pack(packs.find(t->pack_contents()),swp);
                    break;
                case TypeCategory::FUNCTION_POINTER:
                    trace_fp(functions.find(t->function_signature()),swp);
                    break;
                case TypeCategory::POINTER:
                    trace_ptr(pointers.find(&t->pointee()),swp);
                    break;
                default:;
            }
        }
        void trace_pack(packs_t::const_iterator it,LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper& swp) const{
            packs_t::node_type node{packs.extract(it)};
            swp.trace(node.mapped());
            for(const TypeInfo*& t : node.key().arr){
                trace_type(t,swp);
            }
            packs.insert(std::move(node));
        }
        void trace_fp(funcs_t::const_iterator it,LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper& swp) const{
            funcs_t::node_type node{functions.extract(it)};
            swp.trace(node.mapped());
            trace_type(node.key().ret,swp);
            trace_type(node.key().par,swp);
            functions.insert(std::move(node));
        }
        void trace_ptr(ptrs_t::const_iterator it,LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper& swp) const{
            ptrs_t::node_type node{pointers.extract(it)};
            swp.trace(node.mapped());
            trace_type(node.key(),swp);
            trace_type(node.key(),swp);
            pointers.insert(std::move(node));
        }
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
            void trace_compounds(LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper&& swp) const{
                for(auto begin=packs.cbegin(),end=packs.cend();begin != end;){
                    trace_pack(begin++,swp);
                }
                for(auto begin=functions.cbegin(),end=functions.cend();begin != end;){
                    trace_fp(begin++,swp);
                }
                for(auto begin=pointers.cbegin(),end=pointers.cend();begin != end;){
                    trace_ptr(begin++,swp);
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
            const TypeInfo& function_of(FunctionSignature) const;
            const TypeInfo& pointer_to(const TypeInfo&) const;
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
            case TypeCategory::POINTER:
                cppp::muleb128_w<type_id>(dst,pointee().index());
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
            case TypeCategory::POINTER:
                data = &tdb[cppp::muleb128_r<type_id>(buf)];
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
