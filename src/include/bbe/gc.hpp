#pragma once
#include"gcfwd.hpp"
#include"function.hpp"
#include"overload.hpp"
#include"type.hpp"
#include"ast.hpp"
namespace bbe::impl{
    class EntitySweeper{
        LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper tswp;
        LinearMovingGarbageCollectedPool<Function>::Sweeper fswp;
        LinearMovingGarbageCollectedPool<OverloadSet>::Sweeper oswp;
        void _trace_sig(const FunctionSignature& sig){
            trace_type(sig.ret);
        }
        void _trace_pack(const TypePack& pk){
            for(const TraceableReference<TypeInfo> t : pk.arr){
                trace_type(t);
            }
        }
        void _trace_type(const TypeInfo& inf){
            if(!is_type_marked(inf)){
                tswp.mark(inf);
                switch(inf.data.tag()){
                    case TypeCategory::PACK:
                        _trace_pack(inf.data.get<TypeCategory::PACK>());
                        break;
                    case TypeCategory::FUNCTION_POINTER:
                        _trace_sig(inf.data.get<TypeCategory::FUNCTION_POINTER>());
                        break;
                    case TypeCategory::POINTER:
                        trace_type(inf.data.get<TypeCategory::POINTER>());
                        break;
                    default:;
                }
            }
        }
        void _trace_astnode(const ASTNode& nd){
            if(nd.ret != T_ERROR) trace_type(nd.ret);
            if(nd._type == NodeType::FNSYM) trace_function(nd.prim);
            else if(nd._type == NodeType::OVERCALL) trace_overload_set(nd.prim);
            for(const ASTNode& c : nd.children()){
                _trace_astnode(c);
            }
        }
        void _trace_func(const Function& fn){
            if(!is_function_marked(fn)){
                fswp.mark(fn);
                _trace_sig(fn.signature());
                _trace_astnode(fn.ast());
            }
        }
        void _trace_over(const OverloadSet& os){
            for(const TraceableReference<Function>& fr : os.fns){
                trace_function(fr);
            }
        }
        friend class ProjectEntitiesPool;
        EntitySweeper(LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper&& t,LinearMovingGarbageCollectedPool<Function>::Sweeper&& f,LinearMovingGarbageCollectedPool<OverloadSet>::Sweeper&& o) : tswp(std::move(t)), fswp(std::move(f)), oswp(std::move(o)){}
        public:
            bool is_type_marked(const TypeInfo& inf) const{
                return tswp.is_marked(inf);
            }
            bool is_type_marked(type_id ti) const{
                return tswp.is_marked(ti);
            }
            bool is_function_marked(const Function& f) const{
                return fswp.is_marked(f);
            }
            bool is_function_marked(func_id fi) const{
                return fswp.is_marked(fi);
            }
            bool is_overload_set_marked(const OverloadSet& os) const{
                return oswp.is_marked(os);
            }
            bool is_overload_set_marked(over_id oi) const{
                return oswp.is_marked(oi);
            }
            const TypeInfo& new_type_location(const TypeInfo& i) const{
                return tswp.new_location(i);
            }
            TypeInfo& new_type_location(TypeInfo& i) const{
                return tswp.new_location(i);
            }
            type_id new_type_location(type_id i) const{
                return tswp.new_location(i);
            }
            Function& new_function_location(Function& i) const{
                return fswp.new_location(i);
            }
            const Function& new_function_location(const Function& i) const{
                return fswp.new_location(i);
            }
            func_id new_function_location(func_id i) const{
                return fswp.new_location(i);
            }
            OverloadSet& new_overload_set_location(OverloadSet& i) const{
                return oswp.new_location(i);
            }
            const OverloadSet& new_overload_set_location(const OverloadSet& i) const{
                return oswp.new_location(i);
            }
            over_id new_overload_set_location(over_id i) const{
                return oswp.new_location(i);
            }
            void update_type_ref_to_new_location(const TraceableReference<TypeInfo>& tr) const{
                tr.ref = &new_type_location(*tr);
            }
            void update_type_ref_to_new_location(const TraceableReference<TypeInfo>&&) const = delete;
            void update_function_ref_to_new_location(const TraceableReference<Function>& fr) const{
                fr.ref = &new_function_location(*fr);
            }
            void update_function_ref_to_new_location(const TraceableReference<Function>&&) const = delete;
            void update_overload_set_ref_to_new_location(const TraceableReference<OverloadSet>& osr) const{
                osr.ref = &new_overload_set_location(*osr);
            }
            void update_overload_set_ref_to_new_location(const TraceableReference<OverloadSet>&&) const = delete;
            void trace_type(type_id& tid){
                TypeInfo& info = tswp.associated_pool()[tid];
                _trace_type(info);
                tid = info.index();
            }
            void trace_type(TypeInfo*& p){
                TypeInfo& info = *p;
                _trace_type(info);
                p = &new_type_location(info);
            }
            void trace_type(const TypeInfo*& p){
                const TypeInfo& info = *p;
                _trace_type(info);
                p = &new_type_location(info);
            }
            void trace_type(const TraceableReference<TypeInfo>&&) = delete;
            void trace_type(const TraceableReference<TypeInfo>& tr){
                trace_type(tr.ref);
            }
            void trace_function(func_id& fid){
                Function& fn = fswp.associated_pool()[fid];
                _trace_func(fn);
                fid = fn.index();
            }
            void trace_function(Function*& p){
                Function& fn = *p;
                _trace_func(fn);
                p = &new_function_location(fn);
            }
            void trace_function(const Function*& p){
                const Function& fn = *p;
                _trace_func(fn);
                p = &new_function_location(fn);
            }
            void trace_function(const TraceableReference<Function>&&) = delete;
            void trace_function(const TraceableReference<Function>& fr){
                trace_function(fr.ref);
            }
            void trace_overload_set(over_id& oid){
                OverloadSet& os = oswp.associated_pool()[oid];
                _trace_over(os);
                oid = os.index();
            }
            void trace_overload_set(OverloadSet*& p){
                OverloadSet& os = *p;
                _trace_over(os);
                p = &new_overload_set_location(os);
            }
            void trace_overload_set(const OverloadSet*& p){
                const OverloadSet& os = *p;
                _trace_over(os);
                p = &new_overload_set_location(os);
            }
            void trace_overload_set(const TraceableReference<OverloadSet>&&) = delete;
            void trace_overload_set(const TraceableReference<OverloadSet>& osr){
                trace_overload_set(osr.ref);
            }
    };
}
