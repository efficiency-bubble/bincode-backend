#pragma once
#include"commons.hpp"
#include<cstdint>
#include<variant>
#include<limits>
#include<vector>
namespace bbe::impl{
    class ASTNode{
        std::uint64_t _type;
        std::uint64_t prim{0};
        std::vector<ASTNode> chld;
        public:
            constexpr static std::uint64_t NTYPE = std::numeric_limits<std::uint64_t>::max();
            explicit operator bool() const{
                return _type != NTYPE;
            }
            std::uint64_t type() const{
                return _type;
            }
            std::vector<ASTNode>& children(){
                return chld;
            }
            const std::vector<ASTNode>& children() const{
                return chld;
            }
            std::uint64_t getp() const{
                return prim;
            }
            void setp(std::uint64_t p){
                prim = p;
            }
            ASTNode& emplace(std::uint64_t ind,ASTNode&& n){
                return chld[ind] = std::move(n);
            }
            ASTNode() : _type(NTYPE){}
            ASTNode(std::uint64_t tp,std::uint64_t nchld) : _type(tp), chld(nchld){}
            ASTNode(std::uint64_t tp,std::uint64_t prim,std::uint64_t nchld) : _type(tp), prim(prim), chld(nchld){}
    };
}
namespace bbe{
    BBE_EXPORT ASTNode;
}
