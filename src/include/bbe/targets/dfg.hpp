#pragma once
#include"../function.hpp"
#include"../type.hpp"
#include<unordered_map>
#include<cstdint>
#include<vector>
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
    class DataFlowGraph{
        using obs_t = std::vector<const DataNode*>;
        std::deque<DataNode> _nodes;
        std::unordered_map<std::uint32_t,const DataNode*> vars;
        obs_t observables;
        const DataNode* _root;
        const DataNode* compile_node(const ASTNode&,obs_t&);
        const DataNode* compile(const ASTNode&);
        public:
            DataFlowGraph(const Function&);
            const std::deque<DataNode>& nodes() const{
                return _nodes;
            }
            const DataNode* root() const{
                return _root;
            }
            const std::vector<const DataNode*>& envp() const{
                return observables;
            }
    };
}
namespace bbe::targets::dfg{
    BBE_EXPORT DataNode;
    BBE_EXPORT DataFlowGraph;
}

