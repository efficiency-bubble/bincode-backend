#pragma once
#include"gcfwd.hpp"
#include"function.hpp"
#include"entity_pool.hpp"
#include<cppp/int.hpp>
#include<unordered_set>
namespace bbe::impl{
    class OverloadSet : public Entity<over_id>{
        struct phash{
            using is_transparent = void;
            constexpr static std::size_t operator()(TraceableReference<Function> f) noexcept{
                return cppp::assume_cast<std::size_t>(f->signature().parameters().hash().value());
            }
            constexpr static std::size_t operator()(const TypePack& p) noexcept{
                return cppp::assume_cast<std::size_t>(p.hash().value());
            }
        };
        struct peq{
            using is_transparent = void;
            constexpr static std::size_t operator()(TraceableReference<Function> f,TraceableReference<Function> g) noexcept{
                CPPP_ASSERT((f==g) == (f->signature().parameters() == g->signature().parameters()));
                return f == g;
            }
            constexpr static std::size_t operator()(TraceableReference<Function> f,const TypePack& g) noexcept{
                return f->signature().parameters() == g;
            }
            constexpr static std::size_t operator()(const TypePack& g,TraceableReference<Function> f) noexcept{
                return f->signature().parameters() == g;
            }
        };
        std::unordered_set<TraceableReference<Function>,phash,peq> fns;
        friend EntitySweeper;
        public:
            OverloadSet(over_id id,uninitialize_t) : Entity(id){}
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w(dst,fns.size());
                for(const TraceableReference<Function> f : fns){
                    cppp::muleb128_w(dst,f->index());
                }
            }
            void deserialize(cppp::frozen_byte_view& buf,const EntityPool<Function>& fd){
                std::size_t nfns = cppp::muleb128_r<std::size_t>(buf);
                while(nfns--){
                    fns.emplace(fd[cppp::muleb128_r<func_id>(buf)]);
                }
            }
            void add(const Function& fn){
                fns.emplace(fn);
            }
            const Function* match(const TypePack& ptypes) const{
                if(auto it=fns.find(ptypes);it!=fns.end()){
                    return &**it;
                }
                return nullptr;
            }
    };
}
namespace bbe{
    BBE_EXPORT OverloadSet;
}
