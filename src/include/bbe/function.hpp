#pragma once
#include"type.hpp"
#include"ast.hpp"
#include<vector>
namespace bbe::impl{
    class FunctionSignature{
        type_id ret;
        std::vector<type_id> part;
        public:
            FunctionSignature(type_id r,std::vector<type_id>&& p) : ret(std::move(r)), part(std::move(p)){}
            FunctionSignature(type_id r) : ret(std::move(r)){}
            type_id return_type() const{
                return ret;
            }
            const std::vector<type_id>& parameters() const{
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
