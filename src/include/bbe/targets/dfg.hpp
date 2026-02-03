#pragma once
#include"../function.hpp"
#include"../type.hpp"
#include<cppp/variant.hpp>
#include<unordered_map>
#include<cstdint>
#include<vector>
#include<tuple>
#include<deque>
namespace bbe::targets::dfg::impl{
    using namespace bbe::impl;
    class DataNode{
        std::uint32_t op;
        std::uint32_t prim;
        std::vector<const DataNode*> src;
        public:
            DataNode(std::uint32_t op) : op(op){}
            DataNode(std::uint32_t op,std::uint32_t prim) : op(op), prim(prim){}
            DataNode(std::uint32_t op,std::vector<const DataNode*>&& src) : op(op), src(std::move(src)){}
            DataNode(const DataNode&) = delete;
            DataNode(DataNode&&) = default; // only use when nothing points to this
            DataNode& operator=(const DataNode&) = delete;
            DataNode& operator=(DataNode&&) = default;
            ~DataNode(){}
            std::uint32_t operation() const{
                return op;
            }
            std::uint32_t primitive() const{
                return prim;
            }
            const std::vector<const DataNode*>& parents() const{
                return src;
            }
            void emplace(const DataNode* nd){
                src.emplace_back(nd);
            }
    };
    class Fork;
    class Clobbers{
        public:
            using value_type = cppp::heap_variant<Clobbers,Fork,const DataNode*>;
        private:
            std::vector<value_type> subsequence;
            bool ordered;
        public:
            Clobbers(bool ordered=true) : ordered(ordered){}
            const std::vector<value_type>& sequence() const{
                return subsequence;
            }
            bool ordering() const{
                return ordered;
            }
            void push(const DataNode* n){
                subsequence.emplace_back(cppp::emplace_tag<const DataNode*>,n);
            }
            inline Fork& then_fork(const DataNode*);
            Clobbers& then(bool ordered){
                return subsequence.emplace_back(cppp::emplace_tag<Clobbers>,ordered).get<Clobbers>();
            }
    };
    class Fork{
        const DataNode* cond;
        Clobbers lhs;
        Clobbers rhs;
        public:
            Fork(const DataNode* cond) : cond(cond){}
            const DataNode* condition() const{
                return cond;
            }
            const Clobbers& left() const{
                return lhs;
            }
            Clobbers& left(){
                return lhs;
            }
            const Clobbers& right() const{
                return rhs;
            }
            Clobbers& right(){
                return rhs;
            }
    };
    inline Fork& Clobbers::then_fork(const DataNode* on){
        return subsequence.emplace_back(cppp::emplace_tag<Fork>,on).get<Fork>();
    }
    class DataFlowGraph{
        std::deque<DataNode> _nodes;
        std::unordered_map<std::uint32_t,const DataNode*> vars;
        Clobbers clob;
        const DataNode* _root;
        const DataNode* compile(const ASTNode&,Clobbers&);
        public:
            DataFlowGraph(const Function&);
            const Clobbers& clobbers() const{
                return clob;
            }
            const std::deque<DataNode>& nodes() const{
                return _nodes;
            }
            const DataNode* root() const{
                return _root;
            }
    };
}
namespace bbe::targets::dfg{
    BBE_EXPORT DataNode;
    BBE_EXPORT DataFlowGraph;
    BBE_EXPORT Clobbers;
    BBE_EXPORT Fork;
}

