#include<cppp/object-view.hpp>
#include<cppp/string.hpp> // test names
#include<bbe/bbe.hpp>
#include<bbe/inter/dfg.hpp>
#include"test.hpp"
#include<expected>
#include<chrono>
#include<print>
using namespace std::literals;
using test_result_t = std::expected<void,cppp::str>;
struct TestCase{
    cppp::sv name;
    test_result_t(*fn)();
    
};
void test(const cppp::view<const TestCase> cases){
    std::size_t pass=0uz;
    for(std::size_t i=0uz;i<cases.size();++i){
        std::println("[{}/{}] Testing {}"sv,i+1,cases.size(),cppp::cview(cases[i].name));
        try{
            auto result{cases[i].fn()};
            pass += result.has_value();
            if(!result){
                std::println("\x1b[91m{} failed: {}\x1b[0m"sv,cppp::cview(cases[i].name),cppp::cview(result.error()));
            }
        }catch(const std::exception& exc){
            std::println("\x1b[91m{} crashed: {}\x1b[0m"sv,cppp::cview(cases[i].name),exc.what());
        }catch(...){
            std::println("\x1b[91m{} crashed: unknown exception type\x1b[0m"sv,cppp::cview(cases[i].name));
        }
    }
    if(pass<cases.size()){
        std::print("\x1b[33m"sv);
    }
    std::println("{}/{} passed\x1b[0m"sv,pass,cases.size());
}
int main(){
    std::initializer_list<TestCase> test_cases{
        {u8"AST serialization/deserialization"sv,[] -> test_result_t {
            cppp::bytes buf;
            bbe::ASTNode test{133,2};
            test.children().front() = bbe::ASTNode{8,3,0};
            test.serialize(buf);
            cppp::frozen_byte_view reader{buf};
            bbe::ASTNode recover{reader};
            if(!reader.empty()){
                return std::unexpected(u8"Data not fully consumed"s);
            }
            if(recover != test){
                return std::unexpected(u8"Wrong AST deserialized"s);
            }
            return {};
        }},
        {u8"Dfg inter: add values"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            std::uint32_t fn = proj.function_pool().emplace(nullptr,std::vector<const bbe::Type*>{});
            proj.function_pool()[fn].set(cmag(FN_ADD32,pack(u32(1),u32(41))));
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            if(cfp.call(fn,{}).get<bbe::inter::uint32v>().value != 42){
                return std::unexpected(u8"Wrong return value"s);
            }
            return {};
        }},
        {u8"Dfg inter: equality comparison"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            std::uint32_t fn = proj.function_pool().emplace(nullptr,std::vector<const bbe::Type*>{});
            proj.function_pool()[fn].set(cmag(FN_EQ32,pack(u32(42),u32(42))));
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            if(!cfp.call(fn,{}).get<bbe::inter::boolv>().value){
                return std::unexpected(u8"Wrong return value"s);
            }
            return {};
        }}
    };
    test(test_cases);
    return 0;
}
