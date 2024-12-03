#pragma once
#include"commons.hpp"
#include<cppp/bfile.hpp>
#include<cppp/array.hpp>
#include<vector>
namespace bbe::impl{
    struct Import{
        str from;
        str name;
    };
    struct BinaryInfo{
        cppp::frozenbuffer text{};
        cppp::frozenbuffer data{};
        std::vector<Import> imports{};
    };
    void write_binary(cppp::BinaryFile& file,const BinaryInfo& bi);
}
namespace bbe{
    BBE_EXPORT write_binary;
}
