#pragma once
#include"serialization.hpp"
#include"entity_pool.hpp"
#include"idfwd.hpp"
#include"type.hpp"
#include"ast.hpp"
#include<vector>
namespace bbe::impl{
    class ProjectEntitiesPool;
    class ErrorDatabase;
    class Function : public Entity<func_id>{
        FunctionSignature sig;
        ASTNode root;
        public:
            Function(func_id id,uninitialize_for_deserialization_t uninit) : Entity(id), sig(uninit), root(uninit){}
            Function(func_id id,FunctionSignature s) : Entity(id), sig(std::move(s)), root(NodeType::NTYPE){}
            Function(func_id id,ASTNode&& r,FunctionSignature s) : Entity(id), sig(std::move(s)), root(std::move(r)){}
            void deserialize(cppp::frozen_byte_view& buf,const TypeDatabase& tdb){
                sig.deserialize(buf,tdb);
                root.deserialize(buf);
            }
            // can't use EntityPool<Function>::consolidation_map yet, since we're not a complete type. sad.
            void serialize(cppp::bytes& dst,const TypeDatabase::consolidation_map& tcmap,const std::unordered_map<type_id,type_id>& fcmap) const{
                sig.serialize(dst,tcmap);
                root.serialize(dst,fcmap);
            }
            void recalculate_types(ProjectEntitiesPool& p,ErrorDatabase& e){
                root.recursively_recalculate_result_type(p,e,sig);
            }
            void set(ASTNode&& r){
                root = std::move(r);
            }
            const FunctionSignature& signature() const{
                return sig;
            }
            FunctionSignature& signature(){
                return sig;
            }
            ASTNode& ast(){
                return root;
            }
            const ASTNode& ast() const{
                return root;
            }
    };
    class FunctionDatabase{
        using pool_type = EntityPool<Function>;
        pool_type funcs;
        public:
            FunctionDatabase() = default;
            FunctionDatabase(cppp::frozen_byte_view& buf,const TypeDatabase& tdb) : funcs(buf){
                for(func_id i=0;i<funcs.size();++i){
                    funcs[i].deserialize(buf,tdb);
                }
            }
            using consolidation_map = pool_type::consolidation_map;
            consolidation_map make_consolidation_map() const{
                return funcs.make_consolidation_map();
            }
            void serialize(cppp::bytes& dst,const TypeDatabase::consolidation_map& tcmap,const consolidation_map& fcmap) const{
                funcs.serialize(dst,tcmap,fcmap);
            }
            template<typename ...A>
            Function& emplace(A&& ...a){
                return funcs.emplace(std::forward<A>(a)...);
            }
            Function& operator[](func_id i){
                return funcs[i];
            }
            const Function& operator[](func_id i) const{
                return funcs[i];
            }
            void erase(func_id i){
                funcs.pop(i);
            }
            bool has_func(func_id i) const{
                return funcs.occupied(i);
            }
            using iterator = pool_type::iterator;
            using const_iterator = pool_type::const_iterator;
            iterator begin(){
                return funcs.begin();
            }
            iterator end(){
                return funcs.end();
            }
            const_iterator begin() const{
                return funcs.begin();
            }
            const_iterator end() const{
                return funcs.end();
            }
    };
}
namespace bbe{
    BBE_EXPORT FunctionSignature;
    BBE_EXPORT Function;
    BBE_EXPORT func_id;
    BBE_EXPORT FunctionDatabase;
}
