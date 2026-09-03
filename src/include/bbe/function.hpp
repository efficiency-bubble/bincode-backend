#pragma once
#include"serialization.hpp"
#include"hashed_entity_pool.hpp"
#include"idfwd.hpp"
#include"type.hpp"
#include"ast.hpp"
#include<cppp/string.hpp> // compatibility names
#include<vector>
namespace bbe::impl{
    class ProjectEntitiesPool;
    class ErrorDatabase;
    class Function : public HashedEntity<func_id>{
        cppp::str _cname;
        FunctionSignature sig;
        VariableDecls vd;
        ASTNode root;
        public:
            Function(func_id id,uninitialize_t uninit) : HashedEntity(id), sig(uninit), root(uninit){}
            Function(func_id id,FunctionSignature s) : HashedEntity(id), sig(std::move(s)), root(NodeType::NTYPE){}
            Function(func_id id,cppp::sv cn,FunctionSignature s) : HashedEntity(id), _cname(cn), sig(std::move(s)), root(NodeType::NTYPE){}
            Function(func_id id,cppp::str&& cn,FunctionSignature s) : HashedEntity(id), _cname(std::move(cn)), sig(std::move(s)), root(NodeType::NTYPE){}
            void deserialize(cppp::frozen_byte_view& buf,const TypeDatabase& tdb){
                sig.deserialize(buf,tdb);
                std::uint64_t cns = cppp::muleb128_r<std::uint64_t>(buf);
                const char8_t* cnbuf = std::start_lifetime_as_array<char8_t>(buf.read(cns),cns);
                _cname.assign(cnbuf,cns);
                root.deserialize(buf);
            }
            // can't use HashedEntityPool<Function>::consolidation_map yet, since we're not a complete type. sad.
            void serialize(cppp::bytes& dst,const std::unordered_map<type_id,type_id>& fcmap) const{
                sig.serialize(dst);
                cppp::muleb128_w<std::uint64_t>(dst,_cname.size());
                dst.append(std::as_bytes(std::span{_cname}));
                root.serialize(dst,fcmap);
            }
            void trace_types(LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper& swp){
                sig.trace_types(swp);
                root.recursively_trace_types(swp);
            }
            void recalculate_types(ProjectEntitiesPool& p,ErrorDatabase& e){
                root.recursively_recalculate_result_type(p,vd,e,sig);
            }
            void set_cname(cppp::str cn){
                _cname = std::move(cn);
            }
            void set_cname(cppp::sv cn){
                _cname.assign(cn);
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
    class FunctionDatabase{
        using pool_type = HashedEntityPool<Function>;
        pool_type funcs;
        public:
            FunctionDatabase() = default;
            FunctionDatabase(cppp::frozen_byte_view& buf,const TypeDatabase& tdb) : funcs(buf){
                for(func_id i=0;i<funcs.size();++i){
                    funcs[i].deserialize(buf,tdb);
                }
            }
            void trace_types(LinearMovingGarbageCollectedPool<TypeInfo>::Sweeper& swp){
                for(auto& f : funcs){
                    f.trace_types(swp);
                }
            }
            using consolidation_map = pool_type::consolidation_map;
            consolidation_map make_consolidation_map() const{
                return funcs.make_consolidation_map();
            }
            void serialize(cppp::bytes& dst,const consolidation_map& fcmap) const{
                funcs.serialize(dst,fcmap);
            }
            template<typename ...A>
            Function& emplace(A&& ...a){
                return funcs.emplace(std::forward<A>(a)...);
            }
            std::size_t size() const{
                return funcs.size();
            }
            Function& operator[](func_id i){
                return funcs[i];
            }
            const Function& operator[](func_id i) const{
                return funcs[i];
            }
            void erase(func_id i){
                funcs.pop(i);
            }
            bool has_func(func_id i) const{
                return funcs.occupied(i);
            }
            using iterator = pool_type::iterator;
            using const_iterator = pool_type::const_iterator;
            iterator begin(){
                return funcs.begin();
            }
            iterator end(){
                return funcs.end();
            }
            const_iterator begin() const{
                return funcs.begin();
            }
            const_iterator end() const{
                return funcs.end();
            }
    };
}
namespace bbe{
    BBE_EXPORT FunctionSignature;
    BBE_EXPORT Function;
    BBE_EXPORT func_id;
    BBE_EXPORT FunctionDatabase;
}
