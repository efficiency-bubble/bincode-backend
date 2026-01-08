#include<bbe/ast.hpp>
#include<cppp/binary.hpp>
namespace bbe::impl{
    // FIXME: function arg evaluation order
    ASTNode::ASTNode(cppp::frozen_byte_view& b) : chld_and_type(_uninit_tag_t{}), prim(cppp::read<std::uint64_t>(b.read(8uz))){
        chld_and_type.type = cppp::read<std::uint32_t>(b.read(4uz));
        chld_and_type.m = uninitialized_alloc32<ASTNode>(chld_and_type.n = cppp::read<std::uint32_t>(b.read(4uz)));
        for(std::uint32_t i=0;i<chld_and_type.n;++i){
            new(chld_and_type.m+i) ASTNode(b);
        }
    }
    void ASTNode::serialize(cppp::bytes& b) const{
        cppp::write(b.append(8uz),prim);
        cppp::write(b.append(4uz),chld_and_type.type);
        cppp::write(b.append(4uz),chld_and_type.size());
        for(const auto& c : chld_and_type){
            c.serialize(b);
        }
    }
}
