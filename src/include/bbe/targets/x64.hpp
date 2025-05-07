#pragma once
#include"../assembly.hpp"
namespace bbe::targets::x64::impl{
    class X64Compiler : public Compiler{
        public:
            void compile(const Function&,Text&) const override;
    };
}
namespace bbe::targets::x64{
    BBE_EXPORT X64Compiler;
}
