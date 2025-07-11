#pragma once
#include<cstdint>
#define BBE_EXPORT using impl::
#define BBE_DEBUG_STRINGIFY_ENUMERATOR(x) u8 ## #x ## sv
#define BBE_DEBUG_NAMED_ENUM(e,...) enum class e { __VA_ARGS__ }; constexpr cppp::sv stringify_enum(e v){ constexpr static cppp::sv enum_strings[]{ CPPP_FOR_EACH(BBE_DEBUG_STRINGIFY_ENUMERATOR,__VA_ARGS__) }; return enum_strings[static_cast<std::size_t>(v)]; }
namespace bbe::impl{
    using std::uint64_t;
    using uindex = std::size_t;
}
