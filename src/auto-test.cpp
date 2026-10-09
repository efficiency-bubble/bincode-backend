#include<cppp/stringify-enum.hpp>
#include<cppp/object-view.hpp>
#include<cppp/format.hpp>
#include<cppp/string.hpp> // test names
#include<bbe/bbe.hpp>
#include<bbe/inter/dfg.hpp>
#include<expected>
#include<chrono>
#include<print>
#include"test.hpp"
using test_result_t = std::expected<void,cppp::str>;
struct TestCase{
    cppp::sv name;
    test_result_t(*fn)();
};
void test(const cppp::view<const TestCase> cases){
    std::size_t pass=0uz;
    for(std::size_t i=0uz;i<cases.size();++i){
        std::println("[{}/{}] Testing {}"sv,i+1uz,cases.size(),cppp::cview(cases[i].name));
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
            bbe::ASTNode test{NodeType::BOOL,1_u32,bbe::uninitialize};
            test.children().front().initialize(NodeType::PACK,12_u32);
            ASSERT_EQ(test.type(),NodeType::BOOL,"Wrong type");
            ASSERT_EQ(test.children().size(),1_u32,"Wrong nchld");
            ASSERT_EQ(test.children().front().type(),NodeType::PACK,"Wrong type of child");
            ASSERT_EQ(test.children().front().getp32(),12_u32,"Wrong prim of child");
            if(!test.children().front().children().empty()) return std::unexpected(u8"Wrong nchld of child"s);

            bbe::ASTNode test2{std::move(test)};
            ASSERT_EQ(test2.type(),NodeType::BOOL,"Wrong type after move");
            ASSERT_EQ(test2.children().size(),1_u32,"Wrong nchld after move");
            ASSERT_EQ(test2.children().front().type(),NodeType::PACK,"Wrong type of child after move");
            ASSERT_EQ(test2.children().front().getp32(),12_u32,"Wrong prim of child after move");
            if(!test2.children().front().children().empty()) return std::unexpected(u8"Wrong nchld of child after move"s);
            return {};
        }},
        {u8"AST serialization/deserialization"sv,[] -> test_result_t {
            cppp::bytes buf;
            bbe::ASTNode test{NodeType::PACK,2_u32,bbe::uninitialize};
            test.children()[0_u32].initialize({NodeType::COMMA,0_u32,1_u32,bbe::uninitialize});
            test.children()[0_u32].children()[0_u32].initialize();
            test.children()[1_u32].initialize();
            test.serialize(buf);
            for(std::size_t i=0uz;i<buf.size();++i){
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
            VariableDecls vdb;
            ErrorDatabase edb;
            auto an = u32(1024);
            an.recursively_recalculate_result_type(proj,vdb,edb,{proj.types()[T_UINT32],{}});
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            ASSERT_EQ(an.result_type(),T_UINT32,"Wrong type for uint32 literal");
            return {};
        }},
        {u8"PEP serialization/deserialization"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{proj.types()[T_UINT32],{proj.types()[T_UINT64]}});
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
            ASSERT_EQ(deser.functions().size(),1,"Deserialization changed function count");
            ASSERT_EQ(deser.functions()[0].ast() == fn.ast(),true,"Deserialized AST was changed");
            ASSERT_EQ(deser.functions()[0].cname(),u8"test"sv,"Deserialized function cname was changed");
            ASSERT_EQ(deser.functions()[0].signature().parameters().size(),1uz,"Deserialized function parameter count was changed");
            ASSERT_EQ(deser.functions()[0].signature().parameters()[0uz].index(),T_UINT64,"Deserialized function parameter type was changed");
            ASSERT_EQ(deser.functions()[0].signature().return_type().index(),T_UINT32,"Deserialized function return type was changed");
            ASSERT_EQ(&deser.functions()[0].signature().parameters()[0uz],&deser.types()[T_UINT64],"Deserialized function parameter type address was changed");
            ASSERT_EQ(&deser.functions()[0].signature().return_type(),&deser.types()[T_UINT32],"Deserialized function return type address was changed");
            return {};
        }},
        {u8"Dfg inter: add values"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            bbe::Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{proj.types()[T_UINT32],{}});
            fn.set(call(intrin(INTR_ADDU32,proj),u32(1),u32(41)));
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn.index(),{}).get<bbe::inter::uint32v>().value,42,"Wrong return value");
            return {};
        }},
        {u8"Dfg inter: equality comparison"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{proj.types()[T_BOOL],{}});
            fn.set(call(intrin(INTR_EQU32,proj),u32(42),u32(42)));
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            if(!cfp.call(fn.index(),{}).get<bbe::inter::boolv>().value) return std::unexpected(u8"Wrong return value"s);
            return {};
        }},
        {u8"Dfg inter: pack indexing"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{proj.types()[T_UINT32],{}});
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
            Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{proj.types()[T_UINT32],{}});
            fn.set(havevar(0,u32(307),call(intrin(INTR_ADDU32,proj),u32(2),getvar(0))));
            
            // TODO: only recalc once after we fix the dependency issue
            fn.recalculate_types(proj,edb);
            edb.clear();
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn.index(),{}).get<bbe::inter::uint32v>().value,309,"Wrong return value");
            return {};
        }},
        {u8"Dfg inter: comma"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            bbe::Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{proj.types()[T_UINT32],{}});
            const bbe::Function& itpr = intrin(INTR_PRU32,proj);
            fn.set(comma(0,call(itpr,u32(0)),call(itpr,u32(1))));
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn.index(),{}).empty(),true,"Non-empty return value");
            return {};
        }},
        {u8"Garbage collection"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            std::size_t n_builtins = proj.types().size();
            const bbe::TypeInfo& ui32 = proj.types()[T_UINT32];
            bbe::Function* fn = &proj.functions().emplace(u8"test"s,FunctionSignature{proj.types().pack_of({proj.types().pack_of({ui32,ui32}),ui32}),{}});
            
            ASSERT_EQ(proj.functions().size(),1,"Wrong func count pre-collect");
            ASSERT_EQ(proj.types().size(),n_builtins + 2,"Wrong type count pre-collect");
            EntitySweeper swp{proj.begin_gc()};
            swp.trace_function(fn);
            proj.end_gc(std::move(swp));
            ASSERT_EQ(proj.functions().size(),1,"Wrong func count post-nop-collect");
            ASSERT_EQ(proj.types().size(),n_builtins + 2,"Wrong type count post-nop-collect");
            proj.end_gc(proj.begin_gc());
            ASSERT_EQ(proj.types().size(),n_builtins,"Wrong type count post-collect");
            ASSERT_EQ(ui32.size(),4,"ui32 ref was invalidated: wrong size");
            return {};
        }},
        {u8"Dfg inter: pointers"sv,[] -> test_result_t {
            bbe::ProjectEntitiesPool proj;
            bbe::ErrorDatabase edb;
            bbe::Function& fn = proj.functions().emplace(u8"test"s,FunctionSignature{proj.types()[T_VOID],{proj.types()[T_UINT32]}});
            fn.set(deref(addrof(u32(5))));
            fn.recalculate_types(proj,edb);
            ASSERT_EQ(edb.empty(),true,"Errors reported from type inference");
            
            bbe::inter::dfg::CompiledFunctionPool cfp{proj};
            ASSERT_EQ(cfp.call(fn.index(),{}).get<bbe::inter::uint32v>().value,5,"Wrong return value");
            return {};
        }}
    };
    test(test_cases);
    return 0;
}
