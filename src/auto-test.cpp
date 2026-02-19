#include<cppp/object-view.hpp>
#include<cppp/string.hpp> // test names
#include<bbe/bbe.hpp>
#include<bbe/inter/dfg.hpp>
#include<bbe/targets/yasbepl.hpp>
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
template<typename T>
cppp::str to_string(T v){
    if constexpr(std::convertible_to<cppp::str,T>){
        return v;
    }else if constexpr(std::is_enum_v<T>){
        return to_string(std::to_underlying(v));
    }else{
        return cppp::tou8(std::to_string(v));
    }
}
#define ASSERT_EQ(p,q,msg) if(auto r=(p);r!=q) return std::unexpected(u8 ## msg ## s + u8": "s + to_string(r) + u8" != "s + to_string(q));else static_cast<void>(0)
int main(){
    std::initializer_list<TestCase> test_cases{
        {u8"AST construct and move"sv,[] -> test_result_t {
            bbe::ASTNode test{NodeType::BOOL,1};
            test.children().front() = bbe::ASTNode{NodeType::PACK,12,0};
            ASSERT_EQ(test.type(),NodeType::BOOL,"Wrong type");
            ASSERT_EQ(test.children().size(),1,"Wrong nchld");
            ASSERT_EQ(test.children().front().type(),NodeType::PACK,"Wrong type of child");
            ASSERT_EQ(test.children().front().getp32(),12,"Wrong prim of child");
            if(!test.children().front().children().empty()) return std::unexpected(u8"Wrong nchld of child"s);

            bbe::ASTNode test2{std::move(test)};
            ASSERT_EQ(test2.type(),NodeType::BOOL,"Wrong type after move");
            ASSERT_EQ(test2.children().size(),1,"Wrong nchld after move");
            ASSERT_EQ(test2.children().front().type(),NodeType::PACK,"Wrong type of child after move");
            ASSERT_EQ(test2.children().front().getp32(),12,"Wrong prim of child after move");
            if(!test2.children().front().children().empty()) return std::unexpected(u8"Wrong nchld of child after move"s);
            return {};
        }},
        {u8"AST serialization/deserialization"sv,[] -> test_result_t {
            cppp::bytes buf;
            bbe::ASTNode test{NodeType::PACK,2};
            test.children()[0uz] = bbe::ASTNode{NodeType::COMMA,0,1};
            test.children()[0uz].children()[0uz] = bbe::ASTNode{NodeType::NTYPE,0};
            test.children()[1uz] = bbe::ASTNode{NodeType::NTYPE,0};
            test.serialize(buf);
            for(std::size_t i=0;i<buf.size();++i){
                printf("%02x ",(int)buf[i]);
            }
            putchar('\n');
            cppp::frozen_byte_view reader{buf};
            bbe::ASTNode recover{reader};
            if(!reader.empty()) return std::unexpected(u8"Data not fully consumed"s);
            if(recover != test) return std::unexpected(u8"Wrong AST deserialized"s);
            return {};
        }},
        {u8"Dfg inter: add values"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            std::uint32_t fn = proj.function_pool().emplace(nullptr,std::vector<const bbe::Type*>{});
            proj.function_pool()[fn].set(cmag(FN_ADD32,pack(u32(1),u32(41))));
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn,{}).get<bbe::inter::uint32v>().value,42,"Wrong return value");
            return {};
        }},
        {u8"Dfg inter: equality comparison"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            std::uint32_t fn = proj.function_pool().emplace(nullptr,std::vector<const bbe::Type*>{});
            proj.function_pool()[fn].set(cmag(FN_EQ32,pack(u32(42),u32(42))));
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            if(!cfp.call(fn,{}).get<bbe::inter::boolv>().value) return std::unexpected(u8"Wrong return value"s);
            return {};
        }},
        {u8"Dfg inter: pack indexing"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            std::uint32_t fn = proj.function_pool().emplace(nullptr,std::vector<const bbe::Type*>{});
            proj.function_pool()[fn].set(cmag(FN_IPACK,pack(pack(u32(42),u32(41)),u32(1))));
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn,{}).get<bbe::inter::uint32v>().value,41,"Wrong return value");
            return {};
        }},
        {u8"YASBEPL target: basic"sv,[] -> test_result_t {
            bbe::Function fn{nullptr};
            fn.ast() = cmag(FN_ADD32,pack(u32(42),u32(13)));
            bbe::targets::dfg::DataFlowGraph dfg{fn};
            cppp::str buf;
            bbe::targets::yasbepl::compile(dfg,buf);
            ASSERT_EQ(buf,u8">42>13+"s,"Wrong output");
            return {};
        }}
    };
    test(test_cases);
    return 0;
}
