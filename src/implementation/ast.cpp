#include<bbe/ast.hpp>
#include<cppp/binary.hpp>
namespace bbe::impl{
    ASTNode::ASTNode(cppp::frozen_byte_view& b) : _type(cppp::read<std::uint32_t>(b.read(4uz))), prim(cppp::read<std::uint64_t>(b.read(8uz))){
        std::uint64_t nchld = cppp::read<std::uint64_t>(b.read(8uz));
        chld.reserve(nchld);
        while(nchld--){
            chld.emplace_back(b);
        }
    }
    void ASTNode::serialize(cppp::bytes& b) const{
        cppp::write(b.append(4uz),_type);
        cppp::write(b.append(8uz),prim);
        cppp::write(b.append(8uz),static_cast<std::uint64_t>(chld.size()));
        for(const auto& c : chld){
            c.serialize(b);
        }
    }
}
