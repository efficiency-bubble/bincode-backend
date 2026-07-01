#include<bbe/ast.hpp>
#include<bbe/project_entity_pool.hpp>
#include<bbe/serialization.hpp>
#include<cppp/assert.hpp>
namespace bbe::impl{
    constexpr static std::uint32_t VARIABLE = std::numeric_limits<std::uint32_t>::max();
    using namespace cppp::literals;
    
    static std::uint32_t nchld_of(NodeType t){
        switch(t){
            using enum NodeType;
            case UINT32: case UINT64: case BOOL: case ARG: case GETVAR: case BREAK: case UINT32SYM: case FNSYM: case NTYPE:
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
    void ASTNode::deserialize(cppp::frozen_byte_view& buf){
        _type = static_cast<NodeType>(cppp::read<std::uint8_t>(buf));
        prim = cppp::muleb128_r<std::uint32_t>(buf);
        nchld = nchld_of(_type);
        if(nchld == VARIABLE){
            nchld = cppp::muleb128_r<std::uint32_t>(buf);
        }
        if(nchld){
            if(!(_data = reinterpret_cast<std::uint64_t>(std::allocator<ASTNode>::allocate(nchld)))) throw std::bad_alloc();
            for(std::uint32_t i=0;i<nchld;++i){
                new(m()+i) ASTNode(buf);
            }
        }else{
            if(has_extended_data(_type)){
                _data = cppp::read<std::uint64_t>(buf);
            }else{
                _data = 0; // don't leave it uninitialized, to be compare friendly
            }
        }
    }
    void ASTNode::serialize(cppp::bytes& b,const FunctionDatabase::consolidation_map& fcmap) const{
        b.appendl<std::uint8_t>(std::to_underlying(_type));
        cppp::muleb128_w(b,prim);
        std::uint32_t nc = nchld_of(_type);
        if(nc == VARIABLE){
            cppp::muleb128_w(b,nchld);
        }else{
            CPPP_ASSERT(nc == nchld);
        }
        if(has_extended_data(_type)){
            CPPP_ASSERT(nchld == 0);
            b.appendl<std::uint64_t>(_data);
        }
        for(const auto& c : *this){
            c.serialize(b,fcmap);
        }
    }
    void ASTNode::recalculate_result_type(const ProjectEntitiesPool& p,ErrorDatabase& errors,FunctionSignature sig){
        errors.clear(this);
        const auto& tdb = p.types();
        switch(_type){
            using enum NodeType;
            case UINT32: case UINT32SYM:
                ret = tdb.T_UINT32;
                break;
            case UINT64:
                ret = tdb.T_UINT64;
                break;
            case PACK: {
                cppp::fixed_array<const TypeInfo*> a(children().size());
                for(std::uint32_t i=0;i<children().size();++i){
                    if(children()[i].result_type() == tdb.T_ERROR){
                        goto error;
                    }
                    a[i] = &tdb[children()[i].result_type()];
                }
                ret = tdb.pack_of(std::move(a)).index();
                break;
            }
            case COMMA:
                if(std::uint32_t ind=getp32();ind < children().size()){
                    ret = children()[ind].result_type();
                }else{
                    errors.add(this,u8"Comma indexing out of bounds"s);
                    goto error;
                }
                break;
            case PACKIND:
                if(type_id pt = children().front().result_type();pt != tdb.T_ERROR){
                    if(const TypeInfo& t = tdb[pt];t.type() == TypeCategory::PACK){
                        if(getp32() >= t.pack_contents().types().size()){
                            errors.add(this,u8"Pack indexing out of bounds"s);
                            goto error;
                        }
                        ret = t.pack_contents().types()[getp32()]->index();
                    }else{
                        errors.add(this,u8"Cannot index non-pack"s);
                        goto error;
                    }
                }else goto error;
                break;
            case ARG:
                ret = optindex(sig.parameter());
                break;
            case CALL_BUILTIN:
                switch(getp32()){
                    case 0:
                        if(type_id pt = children().front().result_type();pt != tdb.T_ERROR){
                            if(const TypeInfo& t = tdb[pt];t.type() == TypeCategory::FUNCTION_POINTER){
                                type_id at = children()[1uz].result_type();
                                if(at != tdb.T_ERROR && optindex(t.function_signature().parameter()) != at){
                                    errors.add(this,u8"Argument and parameter type mismatch"s);
                                }
                                ret = optindex(t.function_signature().return_type());
                            }else{
                                errors.add(this,u8"Cannot call non-function"s);
                                goto error;
                            }
                        }else goto error;
                        break;
                    case 10:
                    case 20:
                    case 30:
                        if(type_id lt = children()[0uz].result_type();lt != tdb.T_ERROR){
                            if(type_id rt = children()[1uz].result_type();rt != tdb.T_ERROR){
                                if(lt == rt){
                                    if(TypeCategory cat = tdb[lt].type();cat == TypeCategory::UNSIGNED_INTEGRAL || cat == TypeCategory::SIGNED_INTEGRAL){
                                        ret = lt;
                                    }else{
                                        errors.add(this,u8"Non-arithmetic type passed to arithmetic"s);
                                        goto error;
                                    }
                                }else{
                                    errors.add(this,u8"Mismatched operands to arithmetic"s);
                                    goto error;
                                }
                            }else goto error;
                        }else goto error;
                        break;
                        break;
                    case 50:
                    case 51:
                    case 60:
                        ret = tdb.T_BOOL;
                        break;
                    default: throw std::logic_error("AST type inference: unknown magic "s+std::to_string(getp32()));
                }
                break;
            case SETVAR:
                ret = tdb.T_VOID;
                break;
            case GETVAR:
                throw std::logic_error("Unimplemented: getting type of var read"s);
            case BOOL:
                ret = tdb.T_BOOL;
                break;
            case FORK: {
                type_id lht = children()[1uz].result_type();
                type_id rht = children()[2uz].result_type();
                if(lht == rht){
                    ret = lht;
                }else goto error;
                break;
            }
            case FOREVER:
                throw std::logic_error("Unimplemented: getting type of loop"s);
            case FNSYM: {
                if(!p.functions().has_func(getp32())) goto error;
                const FunctionSignature& sig = p.functions()[getp32()].signature();
                ret = tdb.function_of(sig).index();
                break;
            }
            case NTYPE:
                goto error;
            case BREAK:
                ret = tdb.T_VOID;
                break;
        }
        return;
        error:
        ret = tdb.T_ERROR;
    }
}
