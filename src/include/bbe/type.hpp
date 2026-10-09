#pragma once
#include"commons.hpp"
#include"gcfwd.hpp"
#include"uninit.hpp"
#include"idfwd.hpp"
#include"entity_pool.hpp"
#include"serialization.hpp"
#include<cppp/variant.hpp>
#include<cppp/array.hpp>
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
    class TypePack;
    class FunctionSignature;
    class TypeDatabase;
    class TypeInfo;
    enum class TypeCategory : std::uint8_t{
        DTYPE,
        PACK BBE_ANNOTATE(TypePack),
        FUNCTION_POINTER BBE_ANNOTATE(FunctionSignature),
        POINTER BBE_ANNOTATE(TraceableReference<TypeInfo>)
    };
    class TypeInfo : public Entity<type_id>{
        std::uint64_t _size;
        std::uint64_t align;
        type_hash _hash;
        cppp::heap_variant<TypeCategory> data;
        friend TypeDatabase;
        friend EntitySweeper;
        public:
            TypeInfo(type_id id,type_hash hash,cppp::heap_variant<TypeCategory>&& d,std::uint64_t sz,std::uint64_t al) : Entity(id), _size(sz), align(al), _hash(hash), data(std::move(d)){}
            inline TypeInfo(type_id,TypePack&&);
            inline TypeInfo(type_id,FunctionSignature);
            inline TypeInfo(type_id,const TypeInfo&);
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
                return _size + (-_size & (align-1_u64));
            }
            TypeCategory type() const{
                return data.tag();
            }
            type_hash hash() const{
                return _hash;
            }
            const TypePack& pack_contents() const{
                return data.get<TypeCategory::PACK>();
            }
            const FunctionSignature& function_signature() const{
                return data.get<TypeCategory::FUNCTION_POINTER>();
            }
            const TypeInfo& pointee() const{
                return *data.get<TypeCategory::POINTER>();
            }
    };
    class TypePackBuilder{
        cppp::uninitialized_memory<TraceableReference<TypeInfo>> mem;
        friend TypePack;
        public:
            TypePackBuilder(std::size_t n) : mem(n){}
            void emplace(std::size_t i,const TypeInfo& inf){
                mem.emplace_at(i,inf);
            }
            void abandon(std::size_t i){
                mem.destroy_n_from_begin(i);
            }
    };
    class TypePack{
        cppp::fixed_array<TraceableReference<TypeInfo>> arr;
        using view_t = cppp::view<const TypeInfo*>;
        friend EntitySweeper;
        public:
            TypePack(TypePackBuilder&& a) : arr(std::move(a.mem)){}
            TypePack(std::initializer_list<TraceableReference<TypeInfo>> ilis) : arr(ilis){}
            inline TypePack(cppp::frozen_byte_view&,const TypeDatabase&);
            type_hash hash() const{
                if(arr.empty()) return {std::numeric_limits<std::uint64_t>::max()};
                type_hash h = arr[0uz]->hash();
                for(std::size_t i=1uz;i<arr.size();++i){
                    h.combine(arr[i]->hash());
                }
                return h;
            }
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w<std::uint64_t>(dst,arr.size());
                for(const TraceableReference<TypeInfo> p : arr){
                    cppp::muleb128_w<type_id>(dst,p->index());
                }
            }
            class const_iterator{
                const TraceableReference<TypeInfo>* p;
                friend TypePack;
                const_iterator(const TraceableReference<TypeInfo>* p) : p(p){}
                public:
                    using value_type = TypeInfo;
                    const TypeInfo& operator*() const{
                        return **p;
                    }
                    TraceableReference<TypeInfo> operator->() const{
                        return *p;
                    }
                    const_iterator& operator+=(std::ptrdiff_t off){
                        p += off;
                        return *this;
                    }
                    const_iterator operator+(std::ptrdiff_t off){
                        return auto(*this) += off;
                    }
                    const_iterator& operator++(){
                        ++p;
                        return *this;
                    }
                    const_iterator operator++(int){
                        const_iterator dup{*this};
                        ++*this;
                        return dup;
                    }
                    friend bool operator==(const_iterator lhs,const_iterator rhs){
                        return lhs.p == rhs.p;
                    }
            };
            const_iterator begin() const{
                return arr.begin();
            }
            const_iterator end() const{
                return arr.end();
            }
            const cppp::fixed_array<TraceableReference<TypeInfo>>& array() const{
                return arr;
            }
            const TypeInfo& operator[](std::size_t ind) const{
                return *arr[ind];
            }
            std::size_t size() const{
                return arr.size();
            }
            bool operator==(const TypePack& other) const{
                return std::ranges::equal(arr,other.arr);
            }
    };
    class FunctionSignature{
        TraceableReference<TypeInfo> ret;
        union{
            TypePack par;
        };
        friend EntitySweeper;
        public:
            FunctionSignature(uninitialize_t){}
            FunctionSignature(const TypeInfo& r,TypePack&& p) : ret(r), par(std::move(p)){}
            FunctionSignature(cppp::frozen_byte_view& buf,const TypeDatabase& tdb) : FunctionSignature(uninitialize){
                deserialize(buf,tdb);
            }
            FunctionSignature(const FunctionSignature& other) : ret(other.ret), par(other.par){}
            FunctionSignature(FunctionSignature&& other) noexcept : ret(other.ret), par(std::move(other.par)){}
            FunctionSignature& operator=(const FunctionSignature& other){
                ret = other.ret;
                par = other.par;
                return *this;
            }
            FunctionSignature& operator=(FunctionSignature&& other) noexcept{
                ret = other.ret;
                par = std::move(other.par);
                return *this;
            }
            inline void deserialize(cppp::frozen_byte_view&,const TypeDatabase&);
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w<type_id>(dst,ret->index());
                par.serialize(dst);
            }
            type_hash hash() const{
                type_hash h = ret->hash();
                h.combine(par.hash());
                return h;
            }
            void set_return(const TypeInfo& t){
                ret.set(t);
            }
            const TypeInfo& return_type() const{
                return *ret;
            }
            const TypePack& parameters() const{
                return par;
            }
            TypePack& parameters(){
                return par;
            }
            bool operator==(const FunctionSignature& other) const{
                return ret == other.ret && par == other.par;
            }
            ~FunctionSignature(){
                par.~TypePack();
            }
    };
    inline TypeInfo::TypeInfo(type_id id,FunctionSignature sig) : Entity(id), _size(8_u64), align(8_u64), _hash(sig.hash()), data(cppp::in_place_etor<TypeCategory::FUNCTION_POINTER>,sig){}
    inline TypeInfo::TypeInfo(type_id id,const TypeInfo& pe) : Entity(id), _size(8_u64), align(8_u64), _hash(~pe.hash().value()), data(cppp::in_place_etor<TypeCategory::POINTER>,pe){}
    inline TypeInfo::TypeInfo(type_id id,TypePack&& pk) : Entity(id), _size(0_u64), align(1_u64), _hash(pk.hash()), data(cppp::in_place_etor<TypeCategory::PACK>,std::move(pk)){
        for(const TypeInfo& i : pack_contents()){
            _size += i.size();
            align = std::max(align,i.alignment());
        }
    }
    constexpr inline type_id T_VOID = 0;
    constexpr inline type_id T_UINT32 = 1;
    constexpr inline type_id T_INT32 = 2;
    constexpr inline type_id T_UINT64 = 3;
    constexpr inline type_id T_INT64 = 4;
    constexpr inline type_id T_BOOL = 5;
    constexpr inline type_id T_ERROR = std::numeric_limits<type_id>::max();
}
namespace bbe{
    BBE_EXPORT TypePack;
    BBE_EXPORT TypePackBuilder;
    BBE_EXPORT TypeCategory;
    BBE_EXPORT TypeInfo;
    BBE_EXPORT T_VOID;
    BBE_EXPORT T_UINT32;
    BBE_EXPORT T_UINT64;
    BBE_EXPORT T_INT32;
    BBE_EXPORT T_INT64;
    BBE_EXPORT T_BOOL;
    BBE_EXPORT T_ERROR;
}
