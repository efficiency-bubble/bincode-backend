#pragma once
#include"commons.hpp"
#include<cppp/bytearray.hpp>
#include<cppp/object-view.hpp>
#include<algorithm>
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
    // Public API: sequence for accessing children; implementation detail: also packs the ASTNode type to save memory (otherwise it would be wasted on padding)
    class ASTChildren{
        friend ASTNode;
        ASTNode* m;
        std::uint32_t n;
        std::uint32_t type;
        inline void _die();

        // friend_only section (intended to be used by ASTNode)
        ASTChildren(_uninit_tag_t){}
        ASTChildren() : m(nullptr), n(0), type(std::numeric_limits<std::uint32_t>::max()){}
        inline ASTChildren(std::uint32_t n,std::uint32_t t);
        ASTChildren(ASTChildren&& other) : m(std::exchange(other.m,nullptr)), n(other.n), type(other.type){}
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
                return m;
            }
            const ASTNode* begin() const{
                return m;
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
        ASTChildren chld_and_type;
        std::uint64_t prim{0};
        public:
            constexpr static std::uint32_t NTYPE = std::numeric_limits<std::uint32_t>::max();
            explicit operator bool() const{
                return chld_and_type.type != NTYPE;
            }
            std::uint32_t type() const{
                return chld_and_type.type;
            }
            ASTChildren& children(){
                return chld_and_type;
            }
            const ASTChildren& children() const{
                return chld_and_type;
            }
            std::uint64_t getp() const{
                return prim;
            }
            void setp(std::uint64_t p){
                prim = p;
            }
            ASTNode& emplace(std::uint32_t ind,ASTNode&& n){
                return chld_and_type[ind] = std::move(n);
            }
            ASTNode() = default;
            ASTNode(std::uint32_t tp,std::uint32_t nchld) : chld_and_type(nchld,tp){}
            ASTNode(std::uint32_t tp,std::uint64_t prim,std::uint32_t nchld) : chld_and_type(nchld,tp), prim(prim){}
            ASTNode(const ASTNode&) = default;
            ASTNode(ASTNode&& other) : chld_and_type(std::move(other.chld_and_type)), prim(other.prim){}
            ASTNode(cppp::frozen_byte_view&);
            void serialize(cppp::bytes&) const;
            bool operator==(const ASTNode& other) const{
                return prim == other.prim && chld_and_type == other.chld_and_type;
            }
            ASTNode& operator=(const ASTNode&) = default;
            ASTNode& operator=(ASTNode&& other){
                prim = other.prim;
                chld_and_type = std::move(other.chld_and_type);
                return *this;
            }
    };
    inline ASTChildren::ASTChildren(std::uint32_t n,std::uint32_t t) : m(uninitialized_alloc32<ASTNode>(n)), n(n), type(t){
        std::uninitialized_default_construct_n(m,n);
    }
    inline ASTChildren& ASTChildren::operator=(ASTChildren&& other){
        delete std::exchange(m,std::exchange(other.m,nullptr));
        n = std::exchange(other.n,0);
        type = other.type;
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
        return m[ind];
    }
    inline const ASTNode& ASTChildren::operator[](std::uint32_t ind) const{
        return m[ind];
    }
    inline ASTNode* ASTChildren::end(){
        return m+n;
    }
    inline const ASTNode* ASTChildren::end() const{
        return m+n;
    }
    inline ASTNode& ASTChildren::back(){
        return m[n-1];
    }
    inline const ASTNode& ASTChildren::back() const{
        return m[n-1];
    }
    inline bool ASTChildren::operator==(const ASTChildren& other) const{
        return type == other.type && std::ranges::equal(*this,other);
    }
    inline void ASTChildren::_die(){
        if(m){
            std::destroy_n(m,n);
            operator delete(m);
        }
    }
}
namespace bbe{
    BBE_EXPORT ASTNode;
}
