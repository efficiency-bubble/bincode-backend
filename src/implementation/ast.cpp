#include<bbe/ast.hpp>
#include<cppp/binary.hpp>
namespace bbe::impl{
    ASTNode::ASTNode(cppp::frozen_byte_view& b) : chld_and_data(_uninit_tag_t{}), _type{cppp::read<std::uint8_t>(b.read(1uz))}{
        chld_and_data.prim = cppp::read<std::uint32_t>(b.read(4uz));
        if((chld_and_data.n = cppp::read<std::uint32_t>(b.read(4uz)))){
            chld_and_data._data = reinterpret_cast<std::uint64_t>(uninitialized_alloc32<ASTNode>(chld_and_data.n));
            for(std::uint32_t i=0;i<chld_and_data.n;++i){
                new(chld_and_data.m()+i) ASTNode(b);
            }
        }else{
            chld_and_data._data = cppp::read<std::uint64_t>(b.read(8uz));
        }
    }
    void ASTNode::serialize(cppp::bytes& b) const{
        cppp::write(b.append(1uz),std::to_underlying(_type));
        cppp::write(b.append(4uz),chld_and_data.prim);
        cppp::write(b.append(4uz),chld_and_data.n);
        if(chld_and_data.n){
            for(const auto& c : chld_and_data){
                c.serialize(b);
            }
        }else{
            cppp::write(b.append(8uz),chld_and_data._data);
        }
    }
}
