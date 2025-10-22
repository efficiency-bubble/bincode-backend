#pragma once
#include"../function.hpp"
#include"../type.hpp"
#include<cstdint>
#include<vector>
#include<deque>
namespace bbe::targets::dfg::impl{
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
        std::deque<DataNode> _nodes;
        const DataNode* _root;
        public:
            DataFlowGraph(const Function&);
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
}

