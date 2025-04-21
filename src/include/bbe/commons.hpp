#pragma once
#include<cstdint>
#include<cppp/string.hpp>
#define BBE_EXPORT using impl::
namespace bbe::impl{
    using std::uint64_t;
    using uindex = std::size_t;
    using cppp::str;
    using cppp::sv;
    using namespace std::literals::string_literals;
    using namespace std::literals::string_view_literals;
}
