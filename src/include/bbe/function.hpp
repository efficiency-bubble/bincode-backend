#pragma once
#include"type.hpp"
#include"ast.hpp"
#include<vector>
namespace bbe::impl{
    class Function{
        const Type* ret;
        std::vector<const Type*> argt;
        ASTNode root;
        public:
            Function(const Type* r) :  ret(r){}
            Function(const Type* r,std::vector<const Type*>&& a) :  ret(r), argt(std::move(a)){}
            void set(ASTNode&& r){
                root = std::move(r);
            }
            const Type* return_type() const{
                return ret;
            }
            const std::vector<const Type*> argtypes() const{
                return argt;
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
    BBE_EXPORT Function;
}
