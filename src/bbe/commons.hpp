#pragma once
#include<cstdint>
#include<cppp/string.hpp>
#include<cppp/ptr.hpp>
#define BBE_EXPORT using impl::
#define BBE_ENS(n) namespace n = impl::n
namespace bbe::impl{
    using std::uint64_t;
    using uindex = std::size_t;
    using cppp::str;
    using cppp::ptr;
    using cppp::sv;
    using namespace std::literals::string_literals;
    using namespace std::literals::string_view_literals;
}
