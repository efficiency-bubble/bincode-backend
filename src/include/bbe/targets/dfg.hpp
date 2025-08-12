#pragma once
#include"../function.hpp"
#include"../type.hpp"
#include<cstdint>
#include<vector>
#include<deque>
namespace bbe::targets::dfg::impl{
    class DataNode;
    class NodeRef{
        const DataNode* _n;
        std::uint32_t _i;
        public:
            NodeRef() : _n(nullptr){}
            NodeRef(const DataNode& nd,std::uint32_t index=0) : _n(&nd), _i(index){}
            const DataNode& node() const{
                return *_n;
            }
            std::uint32_t index() const{
                return _i;
            }
    };
    class DataNode{
        std::uint32_t op;
        std::uint32_t prim;
        std::vector<NodeRef> src;
        public:
            DataNode(std::uint32_t op) : op(op){}
            DataNode(std::uint32_t op,std::uint32_t prim) : op(op), prim(prim){}
            DataNode(const DataNode&) = delete;
            DataNode(DataNode&&) = delete; // moving breaks pointers to this
            DataNode& operator=(const DataNode&) = delete;
            DataNode& operator=(DataNode&&) = delete;
            ~DataNode(){}
            std::uint32_t operation() const{
                return op;
            }
            std::uint32_t primitive() const{
                return prim;
            }
            const std::vector<NodeRef>& parents() const{
                return src;
            }
            void emplace(NodeRef nd){
                src.emplace_back(nd);
            }
    };
    class DataFlowGraph{
        std::deque<DataNode> _nodes;
        NodeRef _root;
        public:
            DataFlowGraph(const Function&);
            const std::deque<DataNode>& nodes() const{
                return _nodes;
            }
            NodeRef root() const{
                return _root;
            }
    };
}
namespace bbe::targets::dfg{
    BBE_EXPORT NodeRef;
    BBE_EXPORT DataNode;
    BBE_EXPORT DataFlowGraph;
}
