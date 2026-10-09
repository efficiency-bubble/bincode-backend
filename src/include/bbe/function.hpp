#pragma once
#include"gcfwd.hpp"
#include"type.hpp"
#include"ast.hpp"
#include"serialization.hpp"
namespace bbe::impl{
    class Function : public Entity<func_id>{
        cppp::str _cname;
        FunctionSignature sig;
        ASTNode root;
        friend EntitySweeper;
        public:
            constexpr static std::uint32_t INTR_EXTERN = std::numeric_limits<std::uint32_t>::max();
            Function(func_id id,uninitialize_t uninit) : Entity(id), sig(uninit), root(uninit){}
            Function(func_id id,FunctionSignature s) : Entity(id), sig(std::move(s)), root(NodeType::NTYPE){}
            Function(func_id id,cppp::sv cn,FunctionSignature s) : Entity(id), _cname(cn), sig(std::move(s)), root(NodeType::NTYPE){}
            Function(func_id id,cppp::str&& cn,FunctionSignature s) : Entity(id), _cname(std::move(cn)), sig(std::move(s)), root(NodeType::NTYPE){}
            Function(func_id id,std::uint32_t ii,FunctionSignature s) : Entity(id), sig(std::move(s)), root(NodeType::EXTERN_OR_INTRIN,ii){}
            void deserialize(cppp::frozen_byte_view& buf,const TypeDatabase& tdb){
                sig.deserialize(buf,tdb);
                std::size_t cns = cppp::muleb128_r<std::size_t>(buf);
                if(cns){
                    const char8_t* cnbuf = std::start_lifetime_as_array<char8_t>(buf.read(cns-1uz),cns-1uz);
                    _cname.assign(cnbuf,cns-1uz);
                    root.deserialize(buf);
                }else{
                    root.initialize(NodeType::EXTERN_OR_INTRIN,cppp::muleb128_r<std::uint32_t>(buf));
                }
            }
            bool is_intrin() const{
                return root.type() == NodeType::EXTERN_OR_INTRIN && root.getp32() != INTR_EXTERN;
            }
            std::uint32_t intrin() const{
                CPPP_ASSERT(root.type() == NodeType::EXTERN_OR_INTRIN);
                return root.getp32();
            }
            void serialize(cppp::bytes& dst) const{
                sig.serialize(dst);
                if(is_intrin()){
                    cppp::muleb128_w<std::size_t>(dst,0uz);
                    cppp::muleb128_w<std::uint32_t>(dst,root.getp32());
                }else{
                    cppp::muleb128_w<std::size_t>(dst,_cname.size()+1uz);
                    dst.append(std::as_bytes(std::span{_cname}));
                    root.serialize(dst);
                }
            }
            void recalculate_types(ProjectEntitiesPool& p,ErrorDatabase& e){
                VariableDecls vd;
                root.recursively_recalculate_result_type(p,vd,e,sig);
            }
            void set(ASTNode&& r){
                root = std::move(r);
            }
            const FunctionSignature& signature() const{
                return sig;
            }
            FunctionSignature& signature(){
                return sig;
            }
            ASTNode& ast(){
                return root;
            }
            const ASTNode& ast() const{
                return root;
            }
            const cppp::str& cname() const{
                return _cname;
            }
            cppp::str& cname(){
                return _cname;
            }
    };
}
namespace bbe{
    BBE_EXPORT Function;
}
