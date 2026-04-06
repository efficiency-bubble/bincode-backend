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
    static std::uint32_t nchld_of(NodeType t){
        switch(t){
            using enum NodeType;
            case UINT32: case UINT64: case BOOL: case ARGV: case GETVAR: case BREAK: case UINT32SYM: case FNSYM: case NTYPE:
                return 0;
            case SETVAR: case FOREVER: case PACKIND:
                return 1;
            case FORK:
                return 3;
            case PACK: case COMMA: case CALL_BUILTIN:
                return VARIABLE;
            default:
                std::unreachable();
        }
    }
    static bool has_extended_data(NodeType t){
        return t == NodeType::UINT64;
    }
    ASTNode::ASTNode(cppp::frozen_byte_view& b) : _type{cppp::read<std::uint8_t>(b.read(1uz))}{
        prim = uleb128_r<std::uint32_t>(b);
        nchld = nchld_of(_type);
        if(nchld == VARIABLE){
            nchld = uleb128_r<std::uint32_t>(b);
        }
        if(nchld){
            _data = reinterpret_cast<std::uint64_t>(uninitialized_alloc32<ASTNode>(nchld));
            for(std::uint32_t i=0;i<nchld;++i){
                new(m()+i) ASTNode(b);
            }
        }else{
            if(has_extended_data(_type)){
                _data = cppp::read<std::uint64_t>(b.read(8uz));
            }else{
                _data = 0; // don't leave it uninitialized, to be compare friendly
            }
        }
    }
    void ASTNode::serialize(cppp::bytes& b) const{
        cppp::write(b.resb(1uz),std::to_underlying(_type));
        uleb128_w(b,prim);
        std::uint32_t nc = nchld_of(_type);
        if(nc == VARIABLE){
            uleb128_w(b,nchld);
        }else{
            CPPP_ASSERT(nc == nchld);
        }
        if(has_extended_data(_type)){
            CPPP_ASSERT(nchld == 0);
            cppp::write(b.resb(8uz),_data);
        }
        for(const auto& c : *this){
            c.serialize(b);
        }
    }
    void ASTNode::recalculate_result_type(TypeDatabase& tdb){
        switch(_type){
            using enum NodeType;
            case UINT32: case UINT32SYM:
                ret = TypeDatabase::T_UINT32;
                break;
            case UINT64:
                ret = TypeDatabase::T_UINT64;
                break;
            case PACK: {
                cppp::fixed_array<type_id> a(nchld);
                for(std::uint32_t i=0;i<nchld;++i){
                    a[i] = children()[i].result_type();
                }
                ret = tdb.pack_of(std::move(a));
                break;
            }
            case COMMA:
            case PACKIND: 
                ret = tdb[children().front().result_type()].pack_contents().array()[getp32()];
                break;
            case ARGV:
                throw std::logic_error("Unimplemented: getting type of argv"s);
            case CALL_BUILTIN:
                throw std::logic_error("Unimplemented: getting type of builtin-fn"s);
            case SETVAR:
                ret = TypeDatabase::T_VOID;
                break;
            case GETVAR:
                throw std::logic_error("Unimplemented: getting type of var read"s);
            case BOOL:
                ret = TypeDatabase::T_BOOL;
                break;
            case FORK:
                throw std::logic_error("Unimplemented: getting type of fork"s);
            case FOREVER:
                throw std::logic_error("Unimplemented: getting type of loop"s);
            case FNSYM:
                throw std::logic_error("Unimplemented: function types"s);
            case NTYPE:
                ret = TypeDatabase::T_ERROR;
                break;
            case BREAK:
                ret = TypeDatabase::T_VOID;
                break;
        }
    }
}
