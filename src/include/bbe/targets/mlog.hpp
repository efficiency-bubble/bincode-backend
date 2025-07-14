#pragma once
#include"ljf.hpp"
#include<cppp/string.hpp>
#include<variant>
#include<string> // for emitting mlog text
#include<array>
namespace bbe::targets::mlog::impl{
    cppp::str compile(const ljf::ProcedureIC&);
}
namespace bbe::targets::mlog{
    BBE_EXPORT compile;
}
