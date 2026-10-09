#pragma once
#include"type.hpp"
#include"gc.hpp"
#include<unordered_set>
namespace bbe::impl{
    class TypeDatabase{
        std::uint64_t hashcode = 0;
        mutable EntityPool<TypeInfo> infos;
        struct eq_tr{
            bool operator()(TraceableReference<TypeInfo> tr,TraceableReference<TypeInfo> tr2) const{
                return tr == tr2;
            }
            bool operator()(TraceableReference<TypeInfo> tr,const TypePack& pk) const{
                return tr->type() == TypeCategory::PACK && tr->pack_contents() == pk;
            }
            bool operator()(const TypePack& pk,TraceableReference<TypeInfo> tr) const{
                return tr->type() == TypeCategory::PACK && tr->pack_contents() == pk;
            }
            bool operator()(TraceableReference<TypeInfo> tr,FunctionSignature sg) const{
                return tr->type() == TypeCategory::FUNCTION_POINTER && tr->function_signature() == sg;
            }
            bool operator()(FunctionSignature sg,TraceableReference<TypeInfo> tr) const{
                return tr->type() == TypeCategory::FUNCTION_POINTER && tr->function_signature() == sg;
            }
            bool operator()(TraceableReference<TypeInfo> tr,const TypeInfo* p) const{
                return tr->type() == TypeCategory::POINTER && &tr->pointee() == p;
            }
            bool operator()(const TypeInfo* p,TraceableReference<TypeInfo> tr) const{
                return tr->type() == TypeCategory::POINTER && &tr->pointee() == p;
            }
            using is_transparent = void;
        };
        struct hash_tr{
            constexpr static std::size_t operator()(TraceableReference<TypeInfo> r) noexcept{
                return cppp::assume_cast<std::size_t>(r->hash().value());
            }
            constexpr static std::size_t operator()(const TypePack& pk) noexcept{
                return cppp::assume_cast<std::size_t>(pk.hash().value());
            }
            constexpr static std::size_t operator()(FunctionSignature fs) noexcept{
                return cppp::assume_cast<std::size_t>(fs.hash().value());
            }
            constexpr static std::size_t operator()(const TypeInfo* p) noexcept{
                return cppp::assume_cast<std::size_t>(~p->hash().value());
            }
            using is_transparent = void;
        };
        using compounds_t = std::unordered_set<TraceableReference<TypeInfo>,hash_tr,eq_tr>;
        mutable compounds_t compounds;
        constexpr static type_id T_INTRINSIC_END = 6;
        friend class ProjectEntitiesPool;
        friend TypeInfo;
        LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper sweep(){
            LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper swp{infos.sweep()};
            for(type_id i=0;i<T_INTRINSIC_END;++i){
                swp.trace(i);
            }
            return swp;
        }
        void finalize_gc(EntitySweeper& swp){
            compounds_t::const_iterator it = compounds.begin();
            const compounds_t::const_iterator done = compounds.end();
            while(it != done){
                if(swp.is_marked(**it)){
                    swp.trace_type(*it);
                    ++it;
                }else{
                    it = compounds.erase(it);
                }
            }
        }
        public:
            TypeDatabase(){
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,0_u64,0_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,4_u64,4_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,4_u64,4_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,8_u64,8_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,8_u64,8_u64);
                emplace(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,1_u64,1_u64);
            }
            TypeDatabase(cppp::frozen_byte_view& buf) : infos(T_INTRINSIC_END,buf){
                infos[T_VOID].initialize(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,0_u64,0_u64);
                infos[T_UINT32].initialize(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,4_u64,4_u64);
                infos[T_INT32].initialize(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,4_u64,4_u64);
                infos[T_UINT64].initialize(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,8_u64,8_u64);
                infos[T_INT64].initialize(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,8_u64,8_u64);
                infos[T_BOOL].initialize(hashcode++,cppp::in_place_etor<TypeCategory::DTYPE>,1_u64,1_u64);
                for(type_id i=T_INTRINSIC_END;i<infos.size();++i){
                    infos[i].deserialize(buf,*this);
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
            const TypeInfo& pack_of(TypePack&&) const;
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
    };
    inline TypePack::TypePack(cppp::frozen_byte_view& buf,const TypeDatabase& tdb) : arr(cppp::muleb128_r<std::uint64_t>(buf)){
        for(TraceableReference<TypeInfo>& p : arr){
            p.set(tdb[cppp::muleb128_r<type_id>(buf)]);
        }
    }
    inline void FunctionSignature::deserialize(cppp::frozen_byte_view& buf,const TypeDatabase& tdb){
        ret.set(tdb[cppp::muleb128_r<type_id>(buf)]);
        new(&par) TypePack(buf,tdb);
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
        switch(static_cast<TypeCategory>(cppp::read<std::uint8_t>(buf))){
            case TypeCategory::PACK:
                data.emplace<TypeCategory::PACK>(TypePack{buf,tdb});
                break;
            case TypeCategory::FUNCTION_POINTER:
                data.emplace<TypeCategory::FUNCTION_POINTER>(FunctionSignature{buf,tdb});
                break;
            case TypeCategory::POINTER:
                data.emplace<TypeCategory::POINTER>(tdb[cppp::muleb128_r<type_id>(buf)]);
                break;
            default: goto simple;
        }
        tdb.compounds.emplace(*this);
        simple:
    }
}
namespace bbe{
    BBE_EXPORT TypeDatabase;
}
