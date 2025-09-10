#pragma once
#include"../function.hpp"
#include"../type.hpp"
#include"../entity_pool.hpp"
#include<cstdint>
#include<variant>
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
            const std::vector<const DataNode*>& parents() const{
                return src;
            }
            void emplace(const DataNode* nd){
                src.emplace_back(nd);
            }
    };
    struct SequentialClobber;
    struct ParallelClobber{
        std::vector<std::variant<SequentialClobber,const DataNode*>> sequence;
    };
    struct SequentialClobber{
        std::vector<std::variant<ParallelClobber,const DataNode*>> sequence;
    };
    class DataFlowGraph{
        friend class DfgCompiler;
        using index_type = std::uint32_t;
        std::deque<DataNode> _nodes;
        EntityPool<SequentialClobber,index_type> clobberables;
        const DataNode* _root;
        public:
            DataFlowGraph(const Function&);
            const EntityPool<SequentialClobber,index_type>& clobbers() const{
                return clobberables;
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
    BBE_EXPORT SequentialClobber;
    BBE_EXPORT ParallelClobber;
}
