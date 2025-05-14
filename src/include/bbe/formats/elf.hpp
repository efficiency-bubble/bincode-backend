#pragma once
#include"commons.hpp"
#include<cppp/bytearray.hpp>
#include<cppp/string.hpp>
#include"../targets/x64.hpp"
#include<vector>
#include<span>
namespace bbe::formats::elf::impl{
    using namespace bbe::impl;
    using namespace bbe::formats::impl;
    class Nametable{
        cppp::bytes buf;
        public:
            Nametable() : buf{std::byte{0}}{}
            std::uint32_t add(cppp::sv name){
                std::uint32_t location = static_cast<std::uint32_t>(buf.size());
                buf.append(as_bytes(std::span<const char8_t>(name)));
                buf.append(0);
                return location;
            }
            const std::byte* data() const{
                return buf.data();
            }
            std::size_t size() const{
                return buf.size();
            }
    };
    class Elf{
        constexpr static std::size_t SECTION_NAME_TABLE_INDEX = 0uz;
        Nametable section_names;
        constexpr static std::size_t SYMBOL_NAME_TABLE_INDEX = 1uz;
        Nametable symbol_names;
        std::vector<cppp::bytes> section_data;
        struct Section{
            std::uint32_t name;
            std::uint32_t type;
            const std::byte* data; // std::span(nullptr,nonzero_size) is UB.
            std::size_t size;
            std::uint64_t m_addr;
            std::uint64_t align;
            bool load;
            bool write;
            bool exec;
            std::uint64_t entsize;
            std::uint32_t link;
            std::uint32_t info;
        };
        bool wide; // 64-bit if true, else 32-bit
        std::uint64_t entry_point;
        mutable std::vector<Section> sections;
        void add_string_table(cppp::sv name){
            add_section(name,3/* string table */,nullptr,0uz,0,0,false,false,false,0,0);
        }
        public:
            Elf(bool wide=true) : wide(wide){
                using namespace std::string_view_literals;
                add_string_table(u8".shstrtab"sv);
                add_string_table(u8".strtab"sv);
            }
            void add_text(const targets::x64::X64Program& text);
            void add_section(cppp::sv name,std::uint32_t type,const std::byte* data,std::size_t size,std::uint64_t m_addr,std::uint64_t align,bool load=false,bool writable=false,bool executable=false,std::uint64_t entsize=0,std::uint32_t link=0,std::uint32_t info=0){
                sections.emplace_back(section_names.add(name),type,data,size,m_addr,align,load,writable,executable,entsize,link,info);
            }
            cppp::bytes encode() const;
    };
    
}
namespace bbe::formats::elf{
    BBE_EXPORT Elf;
}
