#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<format>
#include<print>
namespace bbe::inter::impl{
    using namespace bbe::impl;
    Value cmag(std::uint32_t magic,const Value& arg){
        const auto& packv = arg.get<pack>().values;
        switch(magic){
            case 10: // add
                return uint32v(packv.front().get<uint32v>().value+packv[1uz].get<uint32v>().value);
            case 25: // pru32
                std::print("{}",packv.front().get<uint32v>().value);
                return {};
            default:
                throw std::logic_error(std::format("inter::cmag(): Unknown magic function {}"sv,magic));
        }
    }
}
