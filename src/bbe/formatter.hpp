#pragma once
#include"ast.hpp"
#include"namespace.hpp"
#include<cppp/virtual.hpp>
#include<variant>
namespace bbe::impl{
    struct SequNode;
    using _snt = std::variant<str,std::vector<SequNode>,const ASTNode*,const Namespace*>;
    struct SequNode : _snt{
        using _snt::_snt;
    };
    class Formatter : public cppp::virtual_class{
        public:
            virtual SequNode format(const ASTNode&) const = 0;
            virtual SequNode format(const Namespace&) const = 0;
            void stringify(str&,const SequNode&) const;
    };
    class SimpleFormatter : public Formatter{
        SequNode format(const Function&) const;
        SequNode format(const Type&) const;
        public:
            SequNode format(const ASTNode&) const override;
            SequNode format(const Namespace&) const override;
    };
}
namespace bbe{
    BBE_EXPORT SequNode;
    BBE_EXPORT Formatter;
    BBE_EXPORT SimpleFormatter;
}
