#pragma once
#include"entity_pool.hpp"
#include"type.hpp"
#include"ast.hpp"
#include<vector>
namespace bbe::impl{
    class ProjectEntitiesPool;
    class ErrorDatabase;
    using func_id = std::uint32_t;
    class Function : public Entity<func_id>{
        FunctionSignature sig;
        ASTNode root;
        public:
            Function(func_id id,FunctionSignature s) : Entity(id), sig(std::move(s)), root(NodeType::NTYPE){}
            Function(func_id id,ASTNode&& r,FunctionSignature s) : Entity(id), sig(std::move(s)), root(std::move(r)){}
            void recalculate_types(ProjectEntitiesPool& p,ErrorDatabase& e){
                root.recursively_recalculate_result_type(p,e,sig);
            }
            void set(ASTNode&& r){
                root = std::move(r);
            }
            const FunctionSignature& signature() const{
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
