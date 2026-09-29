#include<bbe/inter/magic.hpp>
#include<stdexcept>
#include<iostream>
#include<format>
namespace bbe::inter::impl{
    using namespace bbe::impl;
    Value eval_intrin(std::uint32_t intrin,const std::vector<Value>& argv){
        (void)argv;
        switch(intrin){
            case 10: // add32
                return uint32v(argv[0uz].get<uint32v>().value+argv[1uz].get<uint32v>().value);
            case 11: // add32s
                return sint32v(argv[0uz].get<sint32v>().value+argv[1uz].get<sint32v>().value);
            case 20: // sub32
                return uint32v(argv[0uz].get<uint32v>().value-argv[1uz].get<uint32v>().value);
            case 30: // mul32
                return uint32v(argv[0uz].get<uint32v>().value*argv[1uz].get<uint32v>().value);
            case 50: // eq32
                return boolv(argv[0uz].get<uint32v>().value == argv[1uz].get<uint32v>().value);
            case 51: // le32
                return boolv(argv[0uz].get<uint32v>().value <= argv[1uz].get<uint32v>().value);
            case 60: // negbool
                return boolv(!argv.front().get<boolv>().value);
            case 100: // printu32
                std::cout << argv[0uz].get<uint32v>().value << std::endl;
                return {};
            default:
                throw std::logic_error(std::format("inter::eval_intrin(): Unknown intrinsic {}"sv,intrin));
        }
    }
}
