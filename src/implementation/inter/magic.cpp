#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<iostream>
#include<format>
namespace bbe::inter::impl{
    using namespace bbe::impl;
    Value cmag(std::uint32_t magic,const Value& arg){
        const auto& packv = arg.get<pack>().values;
        switch(magic){
            case 10: // add32
                return uint32v(packv.front().get<uint32v>().value+packv[1uz].get<uint32v>().value);
            case 11: // sub32
                return uint32v(packv.front().get<uint32v>().value-packv[1uz].get<uint32v>().value);
            case 25: // pru32
                std::cout << packv.front().get<uint32v>().value << std::flush;
                return {};
            case 50: // eq32
                return boolv(packv.front().get<uint32v>().value == packv[1uz].get<uint32v>().value);
            case 51: // le32
            return boolv(packv.front().get<uint32v>().value <= packv[1uz].get<uint32v>().value);
            case 60: // negbool
                return boolv(!packv.front().get<boolv>().value);
            default:
                throw std::logic_error(std::format("inter::cmag(): Unknown magic function {}"sv,magic));
        }
    }
}
