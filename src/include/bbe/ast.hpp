#pragma once
#include"commons.hpp"
#include<cppp/bytearray.hpp>
#include<cppp/object-view.hpp>
#include<cstdint>
#include<utility>
#include<limits>
#include<vector>
namespace bbe::impl{
    class ASTNode{
        std::uint32_t _type{NTYPE};
        std::uint64_t prim{0};
        std::vector<ASTNode> chld;
        public:
            constexpr static std::uint32_t NTYPE = std::numeric_limits<std::uint32_t>::max();
            explicit operator bool() const{
                return _type != NTYPE;
            }
            std::uint32_t type() const{
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
            ASTNode() = default;
            ASTNode(std::uint32_t tp,std::uint64_t nchld) : _type(tp), chld(nchld){}
            ASTNode(std::uint32_t tp,std::uint64_t prim,std::uint64_t nchld) : _type(tp), prim(prim), chld(nchld){}
            ASTNode(const ASTNode&) = default;
            ASTNode(ASTNode&& other) : _type(std::exchange(other._type,NTYPE)), prim(std::exchange(other.prim,0)), 
            chld(std::move(other.chld)){
                other.chld.clear(); // vector move doesn't guarantee emptiness
            }
            ASTNode(cppp::frozen_byte_view&);
            void serialize(cppp::bytes&) const;
            bool operator==(const ASTNode& other) const{
                return _type == other._type && prim == other.prim && chld == other.chld;
            }
            ASTNode& operator=(const ASTNode&) = default;
            ASTNode& operator=(ASTNode&& other){
                _type = std::exchange(other._type,NTYPE);
                prim = std::exchange(other.prim,0);
                chld = std::move(other.chld);
                other.chld.clear();
                return *this;
            }
    };
}
namespace bbe{
    BBE_EXPORT ASTNode;
}
