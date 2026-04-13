#pragma once
#include"entity_pool.hpp"
#include"type.hpp"
#include"ast.hpp"
#include<vector>
namespace bbe::impl{
    class Function{
        FunctionSignature sig;
        ASTNode root;
        public:
            Function(FunctionSignature s) :  sig(std::move(s)){}
            Function(ASTNode&& r,FunctionSignature s) :  sig(std::move(s)), root(std::move(r)){}
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
    using func_id = std::uint32_t;
    class FunctionDatabase{
        using pool_type = EntityPool<Function,func_id>;
        pool_type funcs;
        public:
            template<typename ...A>
            func_id emplace(A&& ...a){
                return funcs.emplace(std::forward<A>(a)...);
            }
            const Function& operator[](func_id i) const{
                return funcs[i];
            }
            bool has_func(func_id i) const{
                return funcs.occupied(i);
            }
            Function& operator[](func_id i){
                return funcs[i];
            }
            using iterator = pool_type::iterator;
            using const_iterator = pool_type::const_iterator;
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
