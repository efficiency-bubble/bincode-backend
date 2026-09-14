#pragma once
#include<cstdint>
#include<string>
#include<cppp/int.hpp>
#ifdef VERBOSE_LOGGING
#include<cppp/debug.hpp>
#define BBE_DEBUG(...) cppp::debug(__VA_ARGS__)
#else
#define BBE_DEBUG(...)
#endif
#define BBE_EXPORT using impl::
namespace bbe::impl{
    using std::uint64_t;
    using namespace std::literals;
    using namespace cppp::literals;
}
