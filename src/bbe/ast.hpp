#pragma once
#include"commons.hpp"
#include"function.hpp"
#include<assembly/instruction.hpp>
#include<cppp/optional.hpp>
#include<cstdint>
#include<numeric>
#include<utility>
#include<variant>
#include<vector>
#include<set>
#include<map>
namespace bbe::impl{
    struct Value{
        std::uint64_t frame_offset;
    };
    struct data_tag_t{};
    constexpr inline data_tag_t data_tag{};
    class ASTNode{
        std::uint64_t _type;
        using chld_t = std::vector<cppp::optional<ASTNode>>;
        std::variant<
            chld_t,
            std::vector<std::uint32_t>
        > data;
        public:
            std::uint64_t type() const{
                return _type;
            }
            const ASTNode& ugetc(std::uint64_t ind) const{
                return *std::get<chld_t>(data)[ind];
            }
            ASTNode& ugetc(std::uint64_t ind){
                return *std::get<chld_t>(data)[ind];
            }
            const ASTNode* getc(std::uint64_t ind) const{
                return std::get<chld_t>(data)[ind].ptr();
            }
            ASTNode* getc(std::uint64_t ind){
                return std::get<chld_t>(data)[ind].ptr();
            }
            std::uint32_t getp(std::uint64_t ind) const{
                return std::get<std::vector<std::uint32_t>>(data)[ind];
            }
            void setp(std::uint64_t ind,std::uint32_t p){
                std::get<std::vector<std::uint32_t>>(data)[ind] = p;
            }
            template<typename ...A>
            ASTNode& emplace(std::uint64_t ind,A&& ...a){
                std::get<chld_t>(data)[ind].emplace(std::forward<A>(a)...);
                return *std::get<chld_t>(data)[ind];
            }
            ASTNode(std::uint64_t tp,std::uint64_t nchld) : _type(tp), data(std::in_place_type<chld_t>,nchld){}
            ASTNode(std::uint64_t tp,std::uint64_t nchld,data_tag_t) : _type(tp), data(std::in_place_type<std::vector<std::uint32_t>>,nchld){}
    };
}
namespace bbe{
    BBE_EXPORT Value;
    BBE_EXPORT ASTNode;
    BBE_EXPORT data_tag;
}
