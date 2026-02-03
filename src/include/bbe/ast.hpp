#pragma once
#include"commons.hpp"
#include<cppp/bytearray.hpp>
#include<cppp/object-view.hpp>
#include<algorithm>
#include<cassert>
#include<cstdint>
#include<utility>
#include<limits>
#include<memory>
#include<new>
namespace bbe::impl{
    class ASTNode;
    struct _uninit_tag_t{};
    template<typename T>
    T* uninitialized_alloc32(std::uint32_t n){
        if(!n) return nullptr;
        return static_cast<T*>(operator new(sizeof(T)*n));
    }
    enum class NodeType : std::uint8_t{
        UINT32,UINT64,PACK,COMMA,ARGV=5,DEPRECATED_CALL_FN=8,CALL_BUILTIN=9,SETVAR,GETVAR,BOOL=20,FORK,DEPRECATED_BOOLARG,FOREVER=30,BREAK,UINT32SYM=100,DEPRECATED_UINT64SYM=101,DEPRECATED_FNSYM=200,
        NTYPE = 255
    };
    // Public API: sequence for accessing children; implementation detail: also packs the 64-bit data field to save memory (otherwise it would be wasted on padding)
    static_assert(sizeof(std::uintptr_t)==sizeof(std::uint64_t),"Non-64-bit systems unsupported");
    /*
    */
    class ASTChildren{
        friend ASTNode;
        std::uint64_t _data;
        std::uint32_t n;
        std::uint32_t prim;
        inline void _die();
        const ASTNode* m() const{
            return reinterpret_cast<const ASTNode*>(_data);
        }
        ASTNode* m(){
            return reinterpret_cast<ASTNode*>(_data);
        }
        // friend-only section (intended to be used by ASTNode)
        ASTChildren(_uninit_tag_t){}
        ASTChildren() : n(0){}
        inline ASTChildren(std::uint32_t n,std::uint32_t prim=0);
        ASTChildren(ASTChildren&& other) : _data(other._data), n(std::exchange(other.n,0)), prim(other.prim){}
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
                return !n;
            }
            inline ASTNode* end();
            inline const ASTNode* end() const;
            inline ASTNode& back();
            inline const ASTNode& back() const;
            std::uint32_t size() const{
                return n;
            }
            ~ASTChildren(){
                _die();
            }
    };
    class ASTNode{
        ASTChildren chld_and_data;
        NodeType _type;
        public:
            explicit operator bool() const{
                return _type != NodeType::NTYPE;
            }
            NodeType type() const{
                return _type;
            }
            ASTChildren& children(){
                return chld_and_data;
            }
            const ASTChildren& children() const{
                return chld_and_data;
            }
            std::uint32_t getp32() const{
                return chld_and_data.prim;
            }
            std::uint64_t getp64() const{
                return chld_and_data._data;
            }
            void setp32(std::uint32_t p){
                chld_and_data.prim = p;
            }
            void setp64(std::uint64_t p){
                assert(!chld_and_data.n);
                chld_and_data._data = p;
            }
            ASTNode& emplace(std::uint32_t ind,ASTNode&& n){
                return chld_and_data[ind] = std::move(n);
            }
            ASTNode() = default;
            ASTNode(NodeType tp,std::uint32_t nchld) : chld_and_data(nchld), _type(tp){}
            ASTNode(NodeType tp,std::uint32_t prim,std::uint32_t nchld) : chld_and_data(nchld,prim), _type(tp){}
            ASTNode(const ASTNode&) = delete;
            ASTNode(ASTNode&& other) : chld_and_data(std::move(other.chld_and_data)), _type(other._type){}
            ASTNode(cppp::frozen_byte_view&);
            void serialize(cppp::bytes&) const;
            bool operator==(const ASTNode& other) const{
                return _type == other._type && chld_and_data == other.chld_and_data;
            }
            ASTNode& operator=(const ASTNode&) = delete;
            ASTNode& operator=(ASTNode&& other){
                _type = other._type;
                chld_and_data = std::move(other.chld_and_data);
                return *this;
            }
    };
    static_assert(sizeof(ASTNode)<=24,"Regression");
    inline ASTChildren::ASTChildren(std::uint32_t n,std::uint32_t prim) : _data(reinterpret_cast<std::uint64_t>(uninitialized_alloc32<ASTNode>(n))), n(n), prim(prim){
        std::uninitialized_default_construct_n(m(),n);
    }
    inline ASTChildren& ASTChildren::operator=(ASTChildren&& other){
        if(this!=&other){
            _die();
            _data = other._data;
            prim = other.prim;
            n = std::exchange(other.n,0);
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
        return m()+n;
    }
    inline const ASTNode* ASTChildren::end() const{
        return m()+n;
    }
    inline ASTNode& ASTChildren::back(){
        return m()[n-1];
    }
    inline const ASTNode& ASTChildren::back() const{
        return m()[n-1];
    }
    inline bool ASTChildren::operator==(const ASTChildren& other) const{
        if(n != other.n) return false;
        if(prim != other.prim) return false;
        return (!n) || std::ranges::equal(*this,other);
    }
    inline void ASTChildren::_die(){
        if(n){
            std::destroy_n(m(),n);
            operator delete(m());
        }
    }
}
namespace bbe{
    BBE_EXPORT NodeType;
    BBE_EXPORT ASTNode;
}
