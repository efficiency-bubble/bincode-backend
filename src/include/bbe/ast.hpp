#pragma once
#include"commons.hpp"
#include"serialization.hpp"
#include"uninit.hpp"
#include"error.hpp"
#include"idfwd.hpp"
#include"type.hpp"
#include<cppp/object-view.hpp>
#include<cppp/bytearray.hpp>
#include<cppp/assert.hpp>
#include<unordered_map>
#include<algorithm>
#include<cstdint>
#include<utility>
#include<limits>
#include<memory>
#include<new>
namespace bbe::impl{
    class ASTNode;
    enum class NodeType : std::uint8_t{
        UINT32,UINT64,PACK,COMMA,PACKIND,ARG,CALL_BUILTIN=9,SETVAR,GETVAR,BOOL=20,FORK,FOREVER=30,BREAK,UINT32SYM=100,FNSYM=200,
        NTYPE = 255
    };
    // Public API: sequence for accessing children; implementation detail: also packs the 64-bit data field to save memory (otherwise it would be wasted on padding)
    static_assert(sizeof(std::uintptr_t)==sizeof(std::uint64_t),"Non-64-bit systems unsupported");
    
    class ASTChildren : protected std::allocator<ASTNode>{
        protected:
            std::uint64_t _data;
            std::uint32_t nchld;
            inline void _die();
            const ASTNode* m() const{
                return reinterpret_cast<const ASTNode*>(_data);
            }
            ASTNode* m(){
                return reinterpret_cast<ASTNode*>(_data);
            }
            explicit ASTChildren() : nchld(0){}
            ASTChildren(uninitialize_t){}
            inline ASTChildren(std::uint32_t,uninitialize_t);
            ASTChildren(ASTChildren&& other) : _data(other._data), nchld(std::exchange(other.nchld,0)){}
            ASTChildren(const ASTChildren& other) = delete
            #ifndef __INTELLISENSE__ // vscode intellisense/EDG doesn't support delete("reason") yet
            ("Too expensive")
            #endif
            ;
            inline ASTChildren& operator=(ASTChildren&&);
            ASTChildren& operator=(const ASTChildren&) = delete
            #ifndef __INTELLISENSE__
            ("Too expensive")
            #endif
            ;
            inline bool operator==(const ASTChildren&) const;
        public:
            using iterator = ASTNode*;
            using const_iterator = const ASTNode*;
            inline ASTNode& operator[](std::uint32_t);
            inline const ASTNode& operator[](std::uint32_t) const;
            ASTNode* begin(){
                return m();
            }
            const ASTNode* begin() const{
                return m();
            }
            ASTNode& front(){
                return *begin();
            }
            const ASTNode& front() const{
                return *begin();
            }
            bool empty() const{
                return !nchld;
            }
            inline void pop(std::uint32_t i);
            inline void emplace(ASTNode&&);
            inline ASTNode* end();
            inline const ASTNode* end() const;
            inline ASTNode& back();
            inline const ASTNode& back() const;
            std::uint32_t size() const{
                return nchld;
            }
            ~ASTChildren(){
                _die();
            }
    };
    class ProjectEntitiesPool;
    struct null_initialize_t{} constexpr inline null_initialize;
    /*
    Nodes have invalid state when constructed with uninitialize_t, or as a children of a node constructed with children + uninitialize_t
    In this state, you cannot:
    - Query it
    - Assign to it
    - Assign it to something else or move-construct something else with it
    - Destroy it
    You are ONLY permitted to initialize its state with initialize() or deserialize().
    
    Note that resolution of CWG2264 may allow move-assignment and move-construction from an invalid node to work. In this case, a moved-from invalid node is now partially invalid such that it is also valid to destroy or query the children count of (a query which will return 0).
    */
    class ASTNode : ASTChildren{
        std::uint32_t prim;
        type_id ret;
        NodeType _type;
        public:
            explicit operator bool() const{
                return _type != NodeType::NTYPE;
            }
            NodeType type() const{
                return _type;
            }
            ASTChildren& children(){
                return *this;
            }
            const ASTChildren& children() const{
                return *this;
            }
            std::uint32_t getp32() const{
                return prim;
            }
            std::uint64_t getp64() const{
                return _data;
            }
            type_id result_type() const{
                return ret;
            }
            void recalculate_result_type(const ProjectEntitiesPool&,ErrorDatabase&,FunctionSignature);
            void recursively_recalculate_result_type(const ProjectEntitiesPool& p,ErrorDatabase& e,FunctionSignature sig){
                for(auto& c : children()){
                    c.recursively_recalculate_result_type(p,e,sig);
                }
                recalculate_result_type(p,e,sig);
            }
            void setp32(std::uint32_t p){
                prim = p;
            }
            void setp64(std::uint64_t p){
                CPPP_ASSERT(nchld);
                _data = p;
            }
            ASTNode(uninitialize_t uninit) : ASTChildren(uninit){}
            ASTNode(NodeType tp) : ASTChildren(), prim(0), ret(TypeDatabase::T_ERROR), _type(tp){}
            ASTNode(NodeType tp,std::uint32_t nchld,uninitialize_t uninit) : ASTChildren(nchld,uninit), prim(0), ret(TypeDatabase::T_ERROR), _type(tp){}
            ASTNode(NodeType tp,std::uint32_t nchld,null_initialize_t) : ASTNode(tp,nchld,uninitialize){
                while(nchld--){
                    children()[nchld].initialize();
                }
            }
            ASTNode(NodeType tp,std::uint32_t prim) : ASTChildren(), prim(prim), ret(TypeDatabase::T_ERROR), _type(tp){}
            ASTNode(NodeType tp,std::uint32_t prim,std::uint32_t nchld,uninitialize_t uninit) : ASTChildren(nchld,uninit), prim(prim), ret(TypeDatabase::T_ERROR), _type(tp){}
            ASTNode(NodeType tp,std::uint32_t prim,std::uint32_t nchld,null_initialize_t) : ASTNode(tp,prim,nchld,uninitialize){
                while(nchld--){
                    children()[nchld].initialize();
                }
            }
            ASTNode(cppp::frozen_byte_view& buf) : ASTNode(uninitialize){
                deserialize(buf);
            }
            ASTNode(const ASTNode&) = delete;
            ASTNode(ASTNode&&) = default;
            void deserialize(cppp::frozen_byte_view&);
            void serialize(cppp::bytes&,const std::unordered_map<func_id,func_id>&) const;
            void initialize(NodeType type,std::uint32_t p){
                nchld = 0;
                prim = p;
                ret = TypeDatabase::T_ERROR;
                _type = type;
            }
            void initialize(){
                initialize(NodeType::NTYPE,0);
            }
            void initialize(ASTNode&& other){
                _data = other._data;
                nchld = std::exchange(other.nchld,0);
                prim = other.prim;
                ret = other.ret;
                _type = other._type;
            }
            bool operator==(const ASTNode& other) const{
                return (_type == other._type) && (prim == other.prim) && ASTChildren::operator==(other);
            }
            ASTNode& operator=(const ASTNode&) = delete;
            ASTNode& operator=(ASTNode&&) = default;
    };
    #ifndef __INTELLISENSE__ // intellisense doesn't reuse the base class padding, so it always thinks we have a regression
    static_assert(sizeof(ASTNode)<=24,"Regression");
    #endif
    inline ASTChildren::ASTChildren(std::uint32_t n,uninitialize_t uninit) : _data(reinterpret_cast<std::uint64_t>(std::allocator<ASTNode>::allocate(n))), nchld(n){
        std::uninitialized_fill_n(m(),n,uninit);
    }
    inline ASTChildren& ASTChildren::operator=(ASTChildren&& other){
        if(this!=&other){
            _die();
            _data = other._data;
            nchld = std::exchange(other.nchld,0);
        }
        return *this;
    }
    // inline ASTChildren& ASTChildren::operator=(const ASTChildren& other){
    //     _die();
    //     m = uninitialized_alloc32<ASTNode>(other.n);
    //     std::uninitialized_copy_n(other.m,other.n,m);
    //     n = std::exchange(other.n,0);
    //     return *this;
    // }
    inline ASTNode& ASTChildren::operator[](std::uint32_t ind){
        return m()[ind];
    }
    inline const ASTNode& ASTChildren::operator[](std::uint32_t ind) const{
        return m()[ind];
    }
    inline ASTNode* ASTChildren::end(){
        return m()+nchld;
    }
    inline const ASTNode* ASTChildren::end() const{
        return m()+nchld;
    }
    inline ASTNode& ASTChildren::back(){
        return m()[nchld-1];
    }
    inline const ASTNode& ASTChildren::back() const{
        return m()[nchld-1];
    }
    inline bool ASTChildren::operator==(const ASTChildren& other) const{
        if(nchld != other.nchld) return false;
        return (!nchld) || std::ranges::equal(*this,other);
    }
    inline void ASTChildren::_die(){
        if(nchld){
            std::destroy_n(m(),nchld);
            std::allocator<ASTNode>::deallocate(m(),nchld);
        }
    }
    inline void ASTChildren::pop(std::uint32_t indx){
        ASTNode* nmem = std::allocator<ASTNode>::allocate(nchld-1);
        try{
            /*
            indx = 2
            ~~~~~v
            [1 2 3 4 5] = m
            [       ]  = nmem
            
            uninitialized_move_n(m, 2, nmem)
            [1 2 3 4 5] = m
            [1 2    ]  = nmem
            
            uninitialized_move_n(m + 2 + 1, 5 - 2 - 1, nmem+indx)
            [1 2 3 | 4 5] = m
            [1 2   | 4 5]  = nmem
            */
            std::uninitialized_move_n(m(),indx,nmem);
            std::uninitialized_move_n(m()+indx+1,nchld-indx-1,nmem+indx);
        }catch(...){
            std::allocator<ASTNode>::deallocate(nmem,nchld-1);
            throw;
        }
        _die();
        _data = reinterpret_cast<std::uintptr_t>(nmem);
        --nchld;
    }
    inline void ASTChildren::emplace(ASTNode&& nnode){
        ASTNode* nmem = std::allocator<ASTNode>::allocate(nchld+1);
        try{
            std::uninitialized_move_n(m(),nchld,nmem);
            new(nmem+nchld) ASTNode(std::move(nnode));
        }catch(...){
            std::allocator<ASTNode>::deallocate(nmem,nchld+1);
            throw;
        }
        _die();
        _data = reinterpret_cast<std::uintptr_t>(nmem);
        ++nchld;
    }
}
namespace bbe{
    BBE_EXPORT NodeType;
    BBE_EXPORT ASTNode;
    BBE_EXPORT null_initialize_t;
    BBE_EXPORT null_initialize;
}
