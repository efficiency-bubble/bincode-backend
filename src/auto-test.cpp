#include<cppp/object-view.hpp>
#include<cppp/string.hpp> // test names
#include<bbe/bbe.hpp>
#include<bbe/inter/dfg.hpp>
#include<cppp/format.hpp>
#include<bbe/targets/yasbepl.hpp>
#include<cppp/stringify-enum.hpp>
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
cppp::str to_string(cppp::str&& v){
    return std::move(v);
}
cppp::str to_string(cppp::sv v){
    return cppp::str(v);
}
template<typename T> requires(std::is_enum_v<T>)
cppp::str to_string(T v){
    return cppp::str(cppp::stringify_enum(v));
}
template<std::integral T>
cppp::str to_string(T v){
    return cppp::tou8(std::to_string(v));
}
using namespace cppp::literals;
cppp::str to_string(const void* p){
    return cppp::format<u8"{:p}"_ts>(p);
}
cppp::str to_string(bool b){
    return b ? u8"true"s : u8"false"s;
}
#define ASSERT_EQ(p,q,msg) if(auto r=(p);r!=q) return std::unexpected(u8 ## msg ## s + u8": "s + to_string(r) + u8" != "s + to_string(q));else static_cast<void>(0)
int main(){
    std::initializer_list<TestCase> test_cases{
        {u8"AST construct and move"sv,[] -> test_result_t {
            bbe::ASTNode test{NodeType::BOOL,1,bbe::uninitialize};
            test.children().front().initialize(NodeType::PACK,12);
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
            bbe::ASTNode test{NodeType::PACK,2,bbe::uninitialize};
            test.children()[0uz].initialize({NodeType::COMMA,0,1,bbe::uninitialize});
            test.children()[0uz].children()[0uz].initialize();
            test.children()[1uz].initialize();
            test.serialize(buf,{});
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
        {u8"AST type inference"sv,[] -> test_result_t {
            ProjectEntitiesPool proj;
            ErrorDatabase edb;
            auto an = u32(1024);
            an.recursively_recalculate_result_type(proj,edb,{&proj.types()[TypeDatabase::T_UINT32],&proj.types()[TypeDatabase::T_VOID]});
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            ASSERT_EQ(an.result_type(),proj.types().T_UINT32,"Wrong type for uint32 literal");
            return {};
        }},
        {u8"PEP serialization/deserialization"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{&proj.types()[TypeDatabase::T_UINT32],&proj.types()[TypeDatabase::T_VOID]});
            fn.set(pind(pack(u32(42),u32(41)),1));
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            cppp::bytes buf;
            proj.serialize(buf);
            for(std::size_t i=0;i<buf.size();++i){
                printf("%02x ",(int)buf[i]);
            }
            putchar('\n');
            cppp::frozen_byte_view reader{buf};
            bbe::ProjectEntitiesPool deser{reader};
            ASSERT_EQ(deser.functions().has_func(0),true,"Deserialization does not include func id 0");
            ASSERT_EQ(deser.functions()[0].ast() == fn.ast(),true,"Deserialized AST was changed");
            ASSERT_EQ(deser.functions()[0].cname(),u8"test"sv,"Deserialized function cname was changed");
            ASSERT_EQ(deser.functions()[0].signature().parameter()->index(),TypeDatabase::T_VOID,"Deserialized function parameter type was changed");
            ASSERT_EQ(deser.functions()[0].signature().return_type()->index(),TypeDatabase::T_UINT32,"Deserialized function return type was changed");
            ASSERT_EQ(deser.functions()[0].signature().parameter(),&deser.types()[TypeDatabase::T_VOID],"Deserialized function parameter type address was changed");
            ASSERT_EQ(deser.functions()[0].signature().return_type(),&deser.types()[TypeDatabase::T_UINT32],"Deserialized function return type was changed");
            return {};
        }},
        {u8"Dfg inter: add values"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            bbe::Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{&proj.types()[TypeDatabase::T_UINT32],&proj.types()[TypeDatabase::T_VOID]});
            fn.set(cmag(FN_ADD32,u32(1),u32(41)));
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn.index(),{}).get<bbe::inter::uint32v>().value,42,"Wrong return value");
            return {};
        }},
        {u8"Dfg inter: equality comparison"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{&proj.types()[TypeDatabase::T_BOOL],&proj.types()[TypeDatabase::T_VOID]});
            fn.set(cmag(FN_EQ32,u32(42),u32(42)));
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            if(!cfp.call(fn.index(),{}).get<bbe::inter::boolv>().value) return std::unexpected(u8"Wrong return value"s);
            return {};
        }},
        {u8"Dfg inter: pack indexing"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{&proj.types()[TypeDatabase::T_UINT32],&proj.types()[TypeDatabase::T_VOID]});
            fn.set(pind(pack(u32(42),u32(41)),1));
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn.index(),{}).get<bbe::inter::uint32v>().value,41,"Wrong return value");
            return {};
        }},
        {u8"Dfg inter: havevar"sv,[] -> test_result_t {
            
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{&proj.types()[TypeDatabase::T_UINT32],&proj.types()[TypeDatabase::T_VOID]});
            fn.set(havevar(0,u32(307),getvar(0)));
            
            // TODO: check type inference once it actually works
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn.index(),{}).get<bbe::inter::uint32v>().value,307,"Wrong return value");
            return {};
        }}
    };
    test(test_cases);
    return 0;
}
