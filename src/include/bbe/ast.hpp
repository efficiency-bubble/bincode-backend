#pragma once
#include"commons.hpp"
#include"function.hpp"
#include<cstdint>
#include<variant>
#include<limits>
#include<vector>
namespace bbe::impl{
    struct data_tag_t{};
    constexpr inline data_tag_t data_tag{};
    class ASTNode{
        std::uint64_t _type;
        using chld_t = std::vector<ASTNode>;
        std::variant<
            std::monostate,
            chld_t,
            std::vector<std::uint32_t>
        > data;
        public:
            explicit operator bool() const{
                return !std::holds_alternative<std::monostate>(data);
            }
            std::uint64_t type() const{
                return _type;
            }
            const ASTNode& getc(std::uint64_t ind) const{
                return std::get<chld_t>(data)[ind];
            }
            ASTNode& getc(std::uint64_t ind){
                return std::get<chld_t>(data)[ind];
            }
            std::uint32_t getp(std::uint64_t ind) const{
                return std::get<std::vector<std::uint32_t>>(data)[ind];
            }
            void setp(std::uint64_t ind,std::uint32_t p){
                std::get<std::vector<std::uint32_t>>(data)[ind] = p;
            }
            ASTNode& emplace(std::uint64_t ind,ASTNode&& n){
                return std::get<chld_t>(data)[ind] = std::move(n);
            }
            chld_t& children(){
                return std::get<chld_t>(data);
            }
            const chld_t& children() const{
                return std::get<chld_t>(data);
            }
            ASTNode() : _type(0), data(std::in_place_type<std::monostate>){}
            ASTNode(std::uint64_t tp,std::uint64_t nchld) : _type(tp), data(std::in_place_type<chld_t>,nchld){}
            ASTNode(std::uint64_t tp,std::uint64_t nchld,data_tag_t) : _type(tp), data(std::in_place_type<std::vector<std::uint32_t>>,nchld){}
    };
}
namespace bbe{
    BBE_EXPORT ASTNode;
    BBE_EXPORT data_tag;
}
