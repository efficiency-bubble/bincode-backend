#pragma once
#include<cppp/object-view.hpp>
#include<cppp/bytearray.hpp>
#include<cppp/binary.hpp>
#include<concepts>
namespace bbe::impl{
    // Custom version of ULEB128, highest bit of each byte is the opposite of its normal value
    template<std::unsigned_integral T>
    T uleb128_r(cppp::frozen_byte_view& b){
        using namespace cppp::literals;
        T r = 0;
        std::byte v;
        std::uint16_t n = 0;
        do{
            v = b.pop_front();
            // force narrow if promotion happened
            r = static_cast<T>(r | (static_cast<T>(v&0x7f_b) << n));
            n += 7;
        }while((v&0x80_b) == 0_b);
        return r;
    }
    template<std::unsigned_integral T>
    void uleb128_w(cppp::bytes& dst,T v){
        using namespace cppp::literals;
        do{
            dst.append(static_cast<std::byte>(v)&0x7f_b);
            v >>= 7;
        }while(v);
        dst[dst.size()-1] |= 0x80_b;
    }
    struct uninitialize_for_deserialization_t{} constexpr inline uninitialize_for_deserialization;
}
