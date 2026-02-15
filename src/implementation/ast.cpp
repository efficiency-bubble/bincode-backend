#include<bbe/ast.hpp>
#include<cppp/binary.hpp>
#include<cppp/bytearray.hpp>
#include<cppp/assert.hpp>
namespace bbe::impl{
    constexpr static std::uint32_t VARIABLE = std::numeric_limits<std::uint32_t>::max();
    using namespace cppp::literals;
    // Custom version of ULEB128, highest bit of each byte is the opposite of its normal value
    template<typename T>
    static T uleb128_r(cppp::frozen_byte_view& b){
        T r = 0;
        std::byte v;
        std::uint16_t n = 0;
        do{
            v = b.pop_front();
            r |= static_cast<T>(v&0x7f_b) << n;
            n += 7;
        }while((v&0x80_b) == 0_b);
        return r;
    }
    template<typename T>
    static void uleb128_w(cppp::bytes& dst,T v){
        do{
            dst.append(static_cast<std::byte>(v)&0x7f_b);
            v >>= 7;
        }while(v);
        dst[dst.size()-1] |= 0x80_b;
    }
    static std::uint32_t nchld(NodeType t){
        switch(t){
            using enum NodeType;
            case UINT32: case UINT64: case BOOL: case ARGV: case GETVAR: case BREAK: case UINT32SYM: case FNSYM: case NTYPE:
                return 0;
            case SETVAR: case CALL_BUILTIN: case FOREVER:
                return 1;
            case FORK:
                return 3;
            case PACK: case COMMA:
                return VARIABLE;
            default:
                std::unreachable();
        }
    }
    static bool has_extended_data(NodeType t){
        return t == NodeType::UINT64;
    }
    ASTNode::ASTNode(cppp::frozen_byte_view& b) : chld_and_data(_uninit_tag_t{}), _type{cppp::read<std::uint8_t>(b.read(1uz))}{
        chld_and_data.prim = uleb128_r<std::uint32_t>(b);
        chld_and_data.n = nchld(_type);
        if(chld_and_data.n == VARIABLE){
            chld_and_data.n = uleb128_r<std::uint32_t>(b);
        }
        if(chld_and_data.n){
            chld_and_data._data = reinterpret_cast<std::uint64_t>(uninitialized_alloc32<ASTNode>(chld_and_data.n));
            for(std::uint32_t i=0;i<chld_and_data.n;++i){
                new(chld_and_data.m()+i) ASTNode(b);
            }
        }else{
            if(has_extended_data(_type)){
                chld_and_data._data = cppp::read<std::uint64_t>(b.read(8uz));
            }else{
                chld_and_data._data = 0; // don't leave it uninitialized, to be compare friendly
            }
        }
    }
    void ASTNode::serialize(cppp::bytes& b) const{
        cppp::write(b.append(1uz),std::to_underlying(_type));
        uleb128_w(b,chld_and_data.prim);
        std::uint32_t nc = nchld(_type);
        if(nc == VARIABLE){
            uleb128_w(b,chld_and_data.n);
        }else{
            CPPP_ASSERT(nc == chld_and_data.n);
        }
        if(has_extended_data(_type)){
            CPPP_ASSERT(chld_and_data.n == 0);
            cppp::write(b.append(8uz),chld_and_data._data);
        }
        for(const auto& c : chld_and_data){
            c.serialize(b);
        }
    }
}
