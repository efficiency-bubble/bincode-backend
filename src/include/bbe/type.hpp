#pragma once
#include"commons.hpp"
#include"serialization.hpp"
#include"uninit.hpp"
#include"entity_pool.hpp"
#include"idfwd.hpp"
#include<cppp/object-view.hpp>
#include<cppp/variant.hpp>
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
    #ifdef __INTELLISENSE__
    #define BBE_ANNOTATE(t)
    #else
    #define BBE_ANNOTATE(t) [[=^^t]]
    #endif
    struct type_hash{
        std::uint64_t _value;
        public:
            type_hash(std::uint64_t v) : _value(v){}
            type_hash(uninitialize_t){}
            std::uint64_t value() const{
                return _value;
            }
            void combine(type_hash other){
                // https://stackoverflow.com/a/50978188
                _value += 0x9e3779b9_u64 + other._value;
                _value ^= _value >> 32;
                _value *= 0xe9846afb1a615d_u64;
                _value ^= _value >> 32;
                _value *= 0xe9846afb1a615d_u64;
                _value ^= _value >> 28;
            }
    };
    class type_pack;
    class FunctionSignature;
    class TypeDatabase;
    class TypeInfo;
    enum class TypeCategory : std::uint8_t{
        VOID,
        SIGNED_INTEGRAL,
        UNSIGNED_INTEGRAL,
        PACK BBE_ANNOTATE(type_pack),
        FUNCTION_POINTER BBE_ANNOTATE(FunctionSignature),
        POINTER BBE_ANNOTATE(const TypeInfo*)
    };
    class TypeSweeper;
    class TypeInfo : public Entity<type_id>{
        std::uint64_t _size;
        std::uint64_t align;
        type_hash _hash;
        cppp::heap_variant<TypeCategory> data;
        friend TypeDatabase;
        friend TypeSweeper;
        inline void trace_data(TypeSweeper& swp);
        public:
            TypeInfo(type_id id,type_hash hash,cppp::heap_variant<TypeCategory>&& d,std::uint64_t sz,std::uint64_t al) : Entity(id), _size(sz), align(al), _hash(hash), data(std::move(d)){}
            inline TypeInfo(type_id,type_pack&&);
            inline TypeInfo(type_id,FunctionSignature);
            inline TypeInfo(type_id,const TypeInfo*);
            TypeInfo(type_id id,uninitialize_t uninit) : Entity(id), _hash(uninit){}
            void initialize(type_hash h,cppp::heap_variant<TypeCategory>&& t,std::uint64_t sz,std::uint64_t al){
                _size = sz;
                align = al;
                _hash = h;
                data = std::move(t);
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
                return data.tag();
            }
            type_hash hash() const{
                return _hash;
            }
            const type_pack& pack_contents() const{
                return data.get<TypeCategory::PACK>();
            }
            const FunctionSignature& function_signature() const{
                return data.get<TypeCategory::FUNCTION_POINTER>();
            }
            const TypeInfo& pointee() const{
                return *data.get<TypeCategory::POINTER>();
            }
    };
    class TypeSweeper{
        LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper swp;
        public:
            TypeSweeper(LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper&& s) : swp(std::move(s)){}
            bool is_marked(const TypeInfo& inf) const{
                return swp.is_marked(inf);
            }
            const TypeInfo& new_location(const TypeInfo& i) const{
                return swp.new_location(i);
            }
            void trace(type_id& tid){
                bool unmarked = !swp.is_marked(swp.associated_pool()[tid]);
                swp.trace(tid);
                if(unmarked){
                    swp.associated_pool()[tid].trace_data(*this);
                }
            }
            void trace(TypeInfo*& p){
                bool unmarked = !swp.is_marked(*p);
                swp.trace(p);
                if(unmarked){
                    p->trace_data(*this);
                }
            }
            void trace(const TypeInfo*& p){
                TypeInfo* oldp = const_cast<TypeInfo*>(p);
                bool unmarked = !swp.is_marked(*p);
                swp.trace(p); // trace first to mark ourselves, so we don't infinitely recurse
                if(unmarked){
                    oldp->trace_data(*this);
                }
                
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
            type_hash hash() const{
                if(arr.empty()) return {std::numeric_limits<std::uint64_t>::max()};
                type_hash h = arr[0uz]->hash();
                for(std::size_t i=1uz;i<arr.size();++i){
                    h.combine(arr[i]->hash());
                }
                return h;
            }
            void trace_types(TypeSweeper& swp){
                for(const TypeInfo*& p : arr){
                    swp.trace(p);
                }
            }
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
            void trace_types(TypeSweeper& swp){
                swp.trace(ret);
                swp.trace(par);
            }
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w<type_id>(dst,ret->index());
                cppp::muleb128_w<type_id>(dst,par->index());
            }
            type_hash hash() const{
                type_hash h = ret->hash();
                h.combine(par->hash());
                return h;
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
    // sahd, size align hash data
    inline TypeInfo::TypeInfo(type_id id,type_pack&& pk) : Entity(id), _size(0_u64), align(1_u64), _hash(pk.hash()), data(cppp::in_place_etor<TypeCategory::PACK>,std::move(pk)){
        for(const TypeInfo* i : pack_contents().types()){
            _size += i->size();
            align = std::max(align,i->alignment());
        }
    }
    inline TypeInfo::TypeInfo(type_id id,FunctionSignature sig) : Entity(id), _size(8_u64), align(8_u64), _hash(sig.hash()), data(cppp::in_place_etor<TypeCategory::FUNCTION_POINTER>,sig){}
    inline TypeInfo::TypeInfo(type_id id,const TypeInfo* pe) : Entity(id), _size(8_u64), align(8_u64), _hash(~pe->hash().value()), data(cppp::in_place_etor<TypeCategory::POINTER>,pe){}
    class TypeDatabase{
        std::uint64_t hashcode = 0;
        mutable EntityPool<TypeInfo> infos;
        struct TypeReference{
            mutable const TypeInfo* inf;
        };
        struct eq_tr{
            bool operator()(TypeReference tr,TypeReference tr2) const{
                return tr.inf == tr2.inf;
            }
            bool operator()(TypeReference tr,const type_pack& pk) const{
                return tr.inf->type() == TypeCategory::PACK && tr.inf->pack_contents() == pk;
            }
            bool operator()(const type_pack& pk,TypeReference tr) const{
                return tr.inf->type() == TypeCategory::PACK && tr.inf->pack_contents() == pk;
            }
            bool operator()(TypeReference tr,FunctionSignature sg) const{
                return tr.inf->type() == TypeCategory::FUNCTION_POINTER && tr.inf->function_signature() == sg;
            }
            bool operator()(FunctionSignature sg,TypeReference tr) const{
                return tr.inf->type() == TypeCategory::FUNCTION_POINTER && tr.inf->function_signature() == sg;
            }
            bool operator()(TypeReference tr,const TypeInfo* p) const{
                return tr.inf->type() == TypeCategory::POINTER && &tr.inf->pointee() == p;
            }
            bool operator()(const TypeInfo* p,TypeReference tr) const{
                return tr.inf->type() == TypeCategory::POINTER && &tr.inf->pointee() == p;
            }
            using is_transparent = void;
        };
        struct hash_tr{
            constexpr std::size_t operator()(TypeReference r) const noexcept{
                return static_cast<std::size_t>(r.inf->hash().value());
            }
            constexpr std::size_t operator()(const type_pack& pk) const noexcept{
                return static_cast<std::size_t>(pk.hash().value());
            }
            constexpr std::size_t operator()(FunctionSignature fs) const noexcept{
                return static_cast<std::size_t>(fs.hash().value());
            }
            constexpr std::size_t operator()(const TypeInfo* p) const noexcept{
                return static_cast<std::size_t>(~p->hash().value());
            }
            using is_transparent = void;
        };
        using compounds_t = std::unordered_set<TypeReference,hash_tr,eq_tr>;
        mutable compounds_t compounds;
        friend TypeInfo;
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
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::VOID>,0_u64,0_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::UNSIGNED_INTEGRAL>,4_u64,4_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::SIGNED_INTEGRAL>,4_u64,4_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::UNSIGNED_INTEGRAL>,8_u64,8_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::SIGNED_INTEGRAL>,8_u64,8_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::SIGNED_INTEGRAL>,1_u64,1_u64);
            }
            TypeDatabase(cppp::frozen_byte_view& buf) : infos(T_INTRINSIC_END,buf){
                infos[T_VOID].initialize(hashcode++,cppp::in_place_etor<TypeCategory::VOID>,0_u64,0_u64);
                infos[T_UINT32].initialize(hashcode++,cppp::in_place_etor<TypeCategory::UNSIGNED_INTEGRAL>,4_u64,4_u64);
                infos[T_INT32].initialize(hashcode++,cppp::in_place_etor<TypeCategory::SIGNED_INTEGRAL>,4_u64,4_u64);
                infos[T_UINT64].initialize(hashcode++,cppp::in_place_etor<TypeCategory::UNSIGNED_INTEGRAL>,8_u64,8_u64);
                infos[T_INT64].initialize(hashcode++,cppp::in_place_etor<TypeCategory::SIGNED_INTEGRAL>,8_u64,8_u64);
                infos[T_BOOL].initialize(hashcode++,cppp::in_place_etor<TypeCategory::SIGNED_INTEGRAL>,1_u64,1_u64);
                for(type_id i=T_INTRINSIC_END;i<infos.size();++i){
                    infos[i].deserialize(buf,*this);
                }
            }
            TypeSweeper sweep(){
                LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper swp{infos.sweep()};
                for(type_id i=0;i<T_INTRINSIC_END;++i){
                    swp.trace(i);
                }
                return swp;
            }
            void finalize_gc(const TypeSweeper&& swp){
                compounds_t::const_iterator it = compounds.begin();
                const compounds_t::const_iterator done = compounds.end();
                while(it != done){
                    if(swp.is_marked(*it->inf)){
                        it->inf = &swp.new_location(*it->inf);
                        ++it;
                    }else{
                        it = compounds.erase(it);
                    }
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
    inline void TypeInfo::trace_data(TypeSweeper& swp){
        switch(data.tag()){
            case TypeCategory::FUNCTION_POINTER:
                data.get<TypeCategory::FUNCTION_POINTER>().trace_types(swp);
                break;
            case TypeCategory::PACK:
                data.get<TypeCategory::PACK>().trace_types(swp);
                break;
            case TypeCategory::POINTER:
                swp.trace(data.get<TypeCategory::POINTER>());
                break;
            default:;
        }
    }
    inline void TypeInfo::serialize(cppp::bytes& dst) const{
        cppp::muleb128_w<std::uint64_t>(dst,_size);
        cppp::muleb128_w<std::uint64_t>(dst,align);
        dst.appendl(static_cast<std::uint8_t>(data.tag()));
        switch(data.tag()){
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
        TypeCategory cat;
        switch(cat = static_cast<TypeCategory>(cppp::read<std::uint8_t>(buf))){
            case TypeCategory::PACK:
                data.emplace<TypeCategory::PACK>(type_pack{buf,tdb});
                break;
            case TypeCategory::FUNCTION_POINTER:
                data.emplace<TypeCategory::FUNCTION_POINTER>(FunctionSignature{buf,tdb});
                break;
            case TypeCategory::POINTER:
                data.emplace<TypeCategory::POINTER>(&tdb[cppp::muleb128_r<type_id>(buf)]);
                break;
            default: goto simple;
        }
        tdb.compounds.emplace(this);
        simple:
    }
}
namespace bbe{
    BBE_EXPORT type_id;
    BBE_EXPORT type_pack;
    BBE_EXPORT TypeCategory;
    BBE_EXPORT TypeInfo;
    BBE_EXPORT TypeDatabase;
}
