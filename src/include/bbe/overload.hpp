#pragma once
#include"gcfwd.hpp"
#include"function.hpp"
#include"entity_pool.hpp"
#include<cppp/int.hpp>
#include<cppp/functional.hpp>
#include<cppp/meta-functional.hpp>
#include<cppp/meta.hpp>
#include<vector>
namespace bbe::impl{
    class OverloadSet : public Entity<over_id>{
        std::vector<TraceableReference<Function>> fns;
        friend EntitySweeper;
        public:
            struct const_iterator{
                std::vector<TraceableReference<Function>>::const_iterator it;
                friend OverloadSet;
                const_iterator(std::vector<TraceableReference<Function>>::const_iterator it) : it(it){}
                public:
                    using value_type = Function;
                    const Function& operator*() const{
                        return **it;
                    }
                    const Function* operator->() const{
                        return &**it;
                    }
                    const_iterator& operator++(){
                        ++it;
                        return *this;
                    }
                    const_iterator operator++(int){
                        const_iterator dup{*this};
                        ++*this;
                        return dup;
                    }
                    friend bool operator==(const_iterator lhs,const_iterator rhs){
                        return lhs.it == rhs.it;
                    }
            };
            OverloadSet(over_id id) : Entity(id){}
            OverloadSet(over_id id,uninitialize_t) : OverloadSet(id){}
            void serialize(cppp::bytes& dst) const{
                cppp::muleb128_w(dst,fns.size());
                for(const TraceableReference<Function> f : fns){
                    cppp::muleb128_w(dst,f->index());
                }
            }
            void deserialize(cppp::frozen_byte_view& buf,const EntityPool<Function>& fd){
                std::size_t nfns = cppp::muleb128_r<std::size_t>(buf);
                while(nfns--){
                    fns.emplace_back(fd[cppp::muleb128_r<func_id>(buf)]);
                }
            }
            void add(const Function& fn){
                fns.emplace_back(fn);
            }
            void remove(const Function& fn){
                (void)fn;
                auto it = std::ranges::find(fns,&fn,cppp::atop<cppp::addresses<const Function&>,cppp::dereferences<TraceableReference<Function>>>{});
                CPPP_ASSERT(it != fns.end());
                std::ranges::swap(*it,fns.back());
                fns.pop_back();
            }
            const Function* match(const TypePack& ptypes) const{
                for(const TraceableReference<Function> f : fns){
                    if(f->signature().parameters() == ptypes) return f.get();
                }
                // Fancy way:
                // if(auto it = std::ranges::find_if(fns,std::bind_front(std::equal_to<TypePack>{},ptypes),cppp::atop<cppp::atop<cppp::invokes_member<cppp::reflect_member<const TypePack&() const>(&FunctionSignature::parameters)>,cppp::invokes_member<cppp::reflect_member<const FunctionSignature&() const>(&Function::signature)>>,cppp::dereferences<TraceableReference<Function>>>{});it!=fns.end()){
                //     return &**it;
                // }
                return nullptr;
            }
            const_iterator begin() const{
                return fns.begin();
            }
            const_iterator end() const{
                return fns.end();
            }
    };
}
namespace bbe{
    BBE_EXPORT OverloadSet;
}
