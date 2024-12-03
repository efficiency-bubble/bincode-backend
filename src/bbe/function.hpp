#pragma once
#include"type.hpp"
#include"ast.hpp"
#include<vector>
namespace bbe::impl{
    class Function{
        str _name;
        const Type* ret;
        std::vector<const Type*> argt;
        ptr<ASTNode> root;
        public:
            Function(str n,const Type* r,std::vector<const Type*>&& a,ptr<ASTNode>&& root) : _name(std::move(n)), ret(r), argt(std::move(a)), root(std::move(root)){}
            const str& name() const{
                return _name;
            }
            const Type* return_type() const{
                return ret;
            }
            const std::vector<const Type*> argtypes() const{
                return argt;
            }
            ASTNode& ast(){
                return *root;
            }
            const ASTNode& ast() const{
                return *root;
            }
    };
}
namespace bbe{
    BBE_EXPORT Function;
}
