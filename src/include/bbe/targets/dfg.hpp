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
    enum class NodeType : std::uint16_t{
        UINT32,UINT64,PACK,COMMA,PACKIND,ARG,CALL_BUILTIN=9,BOOL=20,FORK,SINT32=30,
        FNSYM=200,
        SEQU=310,
        DUMMY=400,
        
        VOID=65535
    };
    class DataNode{
        NodeType op;
        type_id rtype;
        std::uint32_t prim;
        std::vector<const DataNode*> src;
        public:
            DataNode(NodeType op,type_id rtype) : op(op), rtype(rtype){}
            DataNode(NodeType op,type_id rtype,std::uint32_t prim) : op(op), rtype(rtype), prim(prim){}
            DataNode(NodeType op,type_id rtype,std::vector<const DataNode*>&& src) : op(op), rtype(rtype), src(std::move(src)){}
            DataNode(NodeType op,type_id rtype,std::uint32_t prim,std::vector<const DataNode*>&& src) : op(op), rtype(rtype), prim(prim), src(std::move(src)){}
            DataNode(const DataNode&) = delete;
            DataNode(DataNode&&) = default; // only use when nothing points to this
            DataNode& operator=(const DataNode&) = delete;
            DataNode& operator=(DataNode&&) = default;
            ~DataNode(){}
            NodeType operation() const{
                return op;
            }
            std::uint32_t primitive() const{
                return prim;
            }
            const std::vector<const DataNode*>& parents() const{
                return src;
            }
            void emplace(const DataNode& nd){
                src.emplace_back(&nd);
            }
            type_id return_type() const{
                return rtype;
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
            void setvar(std::uint32_t x,const DataNode& n){
                vars.insert_or_assign(x,&n);
            }
            const DataNode* getvar(std::uint32_t x) const{
                if(auto it=vars.find(x);it!=vars.end()){
                    return it->second;
                }
                if(upstream) return upstream->getvar(x);
                return nullptr;
            }
    };
    struct has_side_effects_t{} constexpr inline has_side_effects;
    class Operation{
        const DataNode* nd;
        const DataNode* side_effect;
        public:
            Operation(const DataNode& nd,const DataNode* se=nullptr) : nd(&nd), side_effect(se){}
            Operation(const DataNode& nd,has_side_effects_t) : Operation(nd,&nd){}
            const DataNode& value() const{
                return *nd;
            }
            const DataNode* side_effects() const{
                return side_effect;
            }
    };
    class DataFlowGraph{
        std::deque<DataNode> _nodes;
        CodeBranch main;
        Operation _root;
        Operation compile(CodeBranch&,const ASTNode&);
        public:
            DataFlowGraph(const bbe::Function&);
            const std::deque<DataNode>& nodes() const{
                return _nodes;
            }
            const Operation& root() const{
                return _root;
            }
    };
    class Function{
        const FunctionSignature* sig;
        DataFlowGraph _dfg;
        public:
            Function(const bbe::Function& fn) : sig(&fn.signature()), _dfg(fn){}
            const DataFlowGraph& dfg() const{
                return _dfg;
            }
            const FunctionSignature& signature() const{
                return *sig;
            }
    };
}
namespace bbe::targets::dfg{
    BBE_EXPORT NodeType;
    BBE_EXPORT DataNode;
    BBE_EXPORT DataFlowGraph;
    BBE_EXPORT Function;
    BBE_EXPORT Operation;
}

