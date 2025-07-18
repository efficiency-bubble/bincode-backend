#pragma once
#include"ljf.hpp"
#include<cppp/string.hpp> // for references to processor links and emitting mlog text
#include<variant>
#include<array>
namespace bbe::targets::mlog::impl{
    struct varref{
        std::uint32_t id;
    };
    struct operation{
        std::uint32_t id;
    };
    struct label{
        std::uint32_t id;
    };
    struct jumpmode{
        std::uint32_t id;
    };
    using Argument = std::variant<varref,operation,label,jumpmode,double,cppp::str>;
    struct Instruction{
        std::uint32_t ins_id;
        std::vector<Argument> argv;
    };
    class ProcedureIC{
        std::vector<Instruction> instructions;
        std::vector<std::uint32_t> labels;
        public:
            ProcedureIC(const ljf::ProcedureIC&);
            cppp::str encode() const;
    };
}
namespace bbe::targets::mlog{
    BBE_EXPORT ProcedureIC;
}
