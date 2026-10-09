#include<bbe/ast.hpp>
#include<bbe/project_entity_pool.hpp>
#include<bbe/serialization.hpp>
#include<cppp/assert.hpp>
#include<cppp/format.hpp>
#include<cppp/int.hpp>
namespace bbe::impl{
    static bool has_extended_data(NodeType t){
        return t == NodeType::UINT64;
    }
    static std::uint32_t nchld_of(NodeType t){
        switch(t){
            using enum NodeType;
            case UINT32: case UINT64: case SINT32: case BOOL: case GETVAR: case UINT32SYM: case FNSYM: case NTYPE: case EXTERN_OR_INTRIN:  case ARG:
                return 0;
            case SETVAR: case PACKIND: case DEREF: case ADDROF:
                return 1;
            case HAVEVAR:
                return 2;
            case FORK:
                return 3;
            case PACK: case COMMA: case CALL: case OVERCALL:
                return VARIABLE;
        }
        cppp::unreachable();
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
                _data = cppp::muleb128_r<std::uint64_t>(buf);
            }else{
                _data = 0; // don't leave it uninitialized, to be compare friendly
            }
        }
    }
    void ASTNode::serialize(cppp::bytes& b) const{
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
            cppp::muleb128_w(b,_data);
        }
        for(const auto& c : *this){
            c.serialize(b);
        }
    }
    void ASTNode::recalculate_result_type(const ProjectEntitiesPool& p,VariableDecls& vd,ErrorDatabase& errors,FunctionSignature sig){
        errors.clear(this);
        const auto& tdb = p.types();
        switch(_type){
            using enum NodeType;
            case UINT32: case UINT32SYM:
                ret = T_UINT32;
                break;
            case UINT64:
                ret = T_UINT64;
                break;
            case SINT32:
                ret = T_INT32;
                break;
            case PACK: {
                TypePackBuilder b(children().size());
                for(std::uint32_t i=0;i<children().size();++i){
                    if(children()[i].result_type() == T_ERROR){
                        b.abandon(i);
                        goto error;
                    }
                    b.emplace(i,tdb[children()[i].result_type()]);
                }
                ret = tdb.pack_of(std::move(b)).index();
                break;
            }
            case COMMA:
                if(std::uint32_t ind=prim;ind < children().size()){
                    ret = children()[ind].result_type();
                }else{
                    errors.add(this,u8"Comma indexing out of bounds"s);
                    goto error;
                }
                break;
            case PACKIND:
                if(type_id pt = children().front().result_type();pt != T_ERROR){
                    if(const TypeInfo& t = tdb[pt];t.type() == TypeCategory::PACK){
                        if(prim >= t.pack_contents().size()){
                            errors.add(this,u8"Pack indexing out of bounds"s);
                            goto error;
                        }
                        ret = t.pack_contents()[prim].index();
                    }else{
                        errors.add(this,u8"Cannot index non-pack"s);
                        goto error;
                    }
                }else goto error;
                break;
            case ARG:
                ret = sig.parameters()[prim].index();
                break;
            case DEREF:
                if(type_id pt = children().front().result_type();pt != T_ERROR){
                    if(tdb[pt].type() == TypeCategory::POINTER){
                        ret = tdb[pt].pointee().index();
                    }else{
                        errors.add(this,u8"Cannot dereference non-pointer"s);
                        goto error;
                    }
                }else goto error;
                break;
            case ADDROF:
                if(type_id pt = children().front().result_type();pt != T_ERROR){
                    ret = tdb.pointer_to(tdb[pt]).index();
                }else goto error;
                break;
            case OVERCALL: {
                if(prim >= p.overloads().size()) goto error;
                const OverloadSet& os = p.overloads()[prim];
                TypePackBuilder tpb{children().size()};
                for(std::uint32_t i=0;i<children().size();++i){
                    if(type_id rt=children()[i].result_type();rt != T_ERROR){
                        tpb.emplace(i,tdb[rt]);
                    }else{
                        tpb.abandon(i);
                        goto error;
                    }
                }
                if(const Function* f=os.match(std::move(tpb))){
                    ret = tdb.function_of(f->signature()).index();
                }else{
                    errors.add(this,u8"No matching overload found"s);
                    goto error;
                }
                break;
            }
            case CALL:
                if(type_id pt = children().front().result_type();pt != T_ERROR){
                    if(const TypeInfo& t = tdb[pt];t.type() == TypeCategory::FUNCTION_POINTER){
                        if(t.function_signature().parameters().size() == children().size() - 1uz){
                            for(std::uint32_t ind=0,indnext;ind<cppp::assume_cast<std::uint32_t>(t.function_signature().parameters().size());ind=indnext){
                                indnext = ind + 1_u32;
                                type_id at = children()[indnext].result_type();
                                if(at != T_ERROR && t.function_signature().parameters()[ind].index() != at){
                                    errors.add(this,cppp::format<u8"Argument and parameter type mismatch at {}"_ts>(ind));
                                }
                            }
                            ret = t.function_signature().return_type().index();
                        }else{
                            errors.add(this,cppp::format<u8"Argument and parameter type count mismatch, #a {} != #p {}"_ts>(children().size() - 1uz,t.function_signature().parameters().size()));
                            goto error;
                        }
                    }else{
                        errors.add(this,u8"Cannot call non-function"s);
                        goto error;
                    }
                }else goto error;
                break;
            case SETVAR:
                // TODO
                ret = T_VOID;
                break;
            case GETVAR:
                if(const ASTNode* p=vd.query(prim)){
                    ret = p->children().front().result_type();
                }else goto error;
                break;
            case HAVEVAR:
                vd.set(prim,*this);
                ret = children()[1_u32].result_type();
                break;
            case BOOL:
                ret = T_BOOL;
                break;
            case FORK: {
                type_id lht = children()[1_u32].result_type();
                type_id rht = children()[2_u32].result_type();
                if(lht == rht){
                    ret = lht;
                }else goto error;
                break;
            }
            case FNSYM: 
                if(prim >= p.functions().size()) goto error;
                ret = tdb.function_of(p.functions()[prim].signature()).index();
                break;
            case NTYPE:
                goto error;
            case EXTERN_OR_INTRIN:
                CPPP_ASSERT(false);
        }
        return;
        error:
        ret = T_ERROR;
    }
}
