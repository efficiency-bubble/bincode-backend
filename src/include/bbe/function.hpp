#pragma once
#include"type.hpp"
#include"ast.hpp"
#include<vector>
namespace bbe::impl{
    class FunctionSignature{
        Type ret;
        std::vector<Type> part;
        public:
            FunctionSignature(Type r,std::vector<Type>&& p) : ret(std::move(r)), part(std::move(p)){}
            FunctionSignature(Type r) : ret(std::move(r)){}
            const Type& return_type() const{
                return ret;
            }
            const std::vector<Type>& parameters() const{
                return part;
            }
    };
    class Function{
        FunctionSignature sig;
        ASTNode root;
        public:
            Function(FunctionSignature s) :  sig(std::move(s)){}
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
}
namespace bbe{
    BBE_EXPORT FunctionSignature;
    BBE_EXPORT Function;
}
