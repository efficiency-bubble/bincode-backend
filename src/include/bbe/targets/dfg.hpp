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
    class CodeBranch{
        const CodeBranch* upstream;
        std::unordered_map<std::uint32_t,const DataNode*> vars;
        public:
            CodeBranch() : upstream(nullptr){}
            CodeBranch(const CodeBranch& up) : upstream(&up){}
            const std::unordered_map<std::uint32_t,const DataNode*>& local_vars() const{
                return vars;
            }
            void setvar(std::uint32_t x,const DataNode* n){
                vars.insert_or_assign(x,n);
            }
            const DataNode* getvar(std::uint32_t x) const{
                if(auto it=vars.find(x);it!=vars.end()){
                    return it->second;
                }
                if(upstream) return upstream->getvar(x);
                return nullptr;
            }
    };
    class DataFlowGraph{
        constexpr static std::uint32_t IO_VAR = std::numeric_limits<std::uint32_t>::max();
        std::deque<DataNode> _nodes;
        CodeBranch main;
        const DataNode* _root;
        const DataNode* compile(CodeBranch&,const ASTNode&);
        public:
            DataFlowGraph(const Function&);
            const std::deque<DataNode>& nodes() const{
                return _nodes;
            }
            const DataNode* stdout_result() const{
                return main.getvar(IO_VAR);
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

