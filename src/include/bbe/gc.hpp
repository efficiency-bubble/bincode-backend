#pragma once
#include"gcfwd.hpp"
#include"function.hpp"
#include"type.hpp"
#include"ast.hpp"
namespace bbe::impl{
    class EntitySweeper{
        LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper tswp;
        LinearMovingGarbageCollectedPool<Function>::Sweeper fswp;
        void _trace_sig(const FunctionSignature& sig){
            trace_type(sig.ret);
        }
        void _trace_pack(const TypePack& pk){
            for(const TraceableReference<TypeInfo>& t : pk.arr){
                trace_type(t);
            }
        }
        void _trace_type(const TypeInfo& inf){
            if(!is_type_marked(inf)){
                tswp.mark(inf);
                switch(inf.data.tag()){
                    case TypeCategory::FUNCTION_POINTER: {
                        _trace_sig(inf.data.get<TypeCategory::FUNCTION_POINTER>());
                        break;
                    }
                    case TypeCategory::PACK:
                        _trace_pack(inf.data.get<TypeCategory::PACK>());
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
        friend class ProjectEntitiesPool;
        EntitySweeper(LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper&& t,LinearMovingGarbageCollectedPool<Function>::Sweeper&& f) : tswp(std::move(t)), fswp(std::move(f)){}
        public:
            bool is_type_marked(const TypeInfo& inf) const{
                return tswp.is_marked(inf);
            }
            bool is_function_marked(const Function& inf) const{
                return fswp.is_marked(inf);
            }
            const TypeInfo& new_type_location(const TypeInfo& i) const{
                return tswp.new_location(i);
            }
            TypeInfo& new_type_location(TypeInfo& i) const{
                return tswp.new_location(i);
            }
            Function& new_function_location(Function& i) const{
                return fswp.new_location(i);
            }
            const Function& new_function_location(const Function& i) const{
                return fswp.new_location(i);
            }
            void update_type_ref_to_new_location(const TraceableReference<TypeInfo>& tr) const{
                tr.ref = &new_type_location(*tr);
            }
            void update_function_ref_to_new_location(const TraceableReference<Function>& tr) const{
                tr.ref = &new_function_location(*tr);
            }
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
            void trace_function(const TraceableReference<Function>& fr){
                trace_function(fr.ref);
            }
    };
}
