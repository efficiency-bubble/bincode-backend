#pragma once
#include"../assembly.hpp"
#include"commons.hpp"
#include<cppp/bytearray.hpp>
#include<cppp/string.hpp>
#include<vector>
#include<span>
namespace bbe::formats::elf::impl{
    using namespace std::string_literals;
    class ElfFile{
        cppp::str names{u8"\0.shstrtab"s};
        struct Section{
            std::uint32_t name;
            std::uint32_t type;
            const std::byte* data; // std::span(nullptr,nonzero_size) is UB.
            std::size_t size;
            std::uint64_t m_addr;
            std::uint64_t align;
            bool write;
            bool exec;
        };
        bool wide; // 64-bit if true, else 32-bit
        std::uint64_t entry_point;
        mutable std::vector<Section> sections;
        std::uint32_t add_name(cppp::sv name){
            std::uint32_t begin = names.size();
            names.append(name);
            return begin;
        }
        public:
            ElfFile(bool wide=true) : wide(wide){
                sections.emplace_back(
                    1,
                    3, // string table
                    nullptr,
                    0uz,
                    0,
                    0,
                    false,
                    false
                );
            }
            void add_text(const Text& text){
                cppp::bytes symtab;
                for(const auto& [name,position] : text.exports()){
                    symtab.appendl(add_name(name));
                    symtab.append((1 << 4) | 2); // HI 1: global binding / linkage, LO 2: function / code export
                    symtab.append(0); // default visibility (allow interposing)
                    symtab.appendl(static_cast<std::uint16_t>(sections.size())); // linked section ID
                    symtab.appendl<std::uint64_t>(position);
                    symtab.appendl<std::uint64_t>(0); // unsized or variably-sized
                }
                add_section(u8".text"s,1/*binary data*/,text.text().data(),text.text().size(),0x1000,0x10,false,true);
                add_section(u8".symtab"s,3/*string table*/,text.text().data(),text.text().size(),0x1000,0x10,false,true);
            }
            void add_section(cppp::sv name,std::uint32_t type,const std::byte* data,std::size_t size,std::uint64_t m_addr,std::uint64_t align,bool writable,bool executable){
                sections.emplace_back(add_name(name),type,data,size,m_addr,align,writable,executable);
            }
            cppp::bytes encode() const{
                sections.front().data = reinterpret_cast<const std::byte*>(names.data());
                sections.front().size = names.size()+1; // + null terminator
                
                cppp::bytes data{
                    0x7F_b,'E'_b,'L'_b,'F'_b, // magic
                    wide?2_b:1_b, // bitness, 2 = 64-bit, 1 = 32-bit
                    1_b, // binary format, 1 = little-endian
                    1_b, // ELF version, 1 = current version
                    0_b, // ABI, 0 = System V ABI
                    0_b // ABI version. Apparently gcc outputs zero so ¯\_(ツ)_/¯
                };
                data.resize(16,0_b); // pad to 16
                data.appendl<std::uint16_t>(1); // type of image, 1 = relocatable file
                data.appendl<std::uint16_t>(62); // machine architecture, 62 = x64
                data.appendl<std::uint32_t>(1); // object file version, 1 = current version
                data.appendl(entry_point);
                data.appendl(std::uint64_t(0)); // PHT offset (none) // TODO
                constexpr std::size_t SHT_OFFSET_POS = 0x28;
                data.skip(8uz); // 8: SHT offset
                data.appendl<std::uint32_t>(0); // processor-specific flags (none supported yet)
                data.appendl<std::uint16_t>(0x40); // size of header
                data.appendl<std::uint16_t>(0x38); // PHT item size
                data.appendl<std::uint16_t>(0); // PHT item count
                data.appendl<std::uint16_t>(0x40); // SHT item size
                std::size_t sht_entry_count = sections.size()+2uz; // null entry + name table
                data.appendl(static_cast<std::uint16_t>(sht_entry_count));
                data.skip(2uz);// 2: SHT item count, section names table index
                // reserve SHT space
                std::size_t sht_offset = data.size();
                data.write(SHT_OFFSET_POS,sht_offset);
                data.skip(0x40uz*sht_entry_count);
                // section data
                std::vector<std::uint64_t> sdaddr;
                std::vector<std::uint64_t> nameindex;
                // sections
                for(const Section& sec : sections){
                    sdaddr.emplace_back(data.size());
                    if(sec.data){
                        data.append({sec.data,sec.size});
                    }
                }
                // SHT
                // sections
                std::size_t index = 0uz;
                for(const Section& sec : sections){
                    data.appendl(static_cast<std::uint32_t>(nameindex[index])); // name index
                    data.appendl(sec.type); // content type
                    std::uint64_t flags{
                        2 // loaded in memory
                    };
                    if(sec.write){
                        flags |= 1; // runtime writable
                    }else if(sec.exec){
                        flags |= 4; // machine code
                    }
                    data.appendl(flags);
                    data.appendl<std::uint64_t>(sec.m_addr); // loaded memory address
                    data.appendl(sdaddr[index]); // file address
                    data.appendl(static_cast<std::uint64_t>(sec.size)); // size
                    data.appendl<std::uint32_t>(0); // link
                    data.appendl<std::uint32_t>(0); // info
                    data.appendl<std::uint32_t>(sec.align); // alignment
                    data.appendl<std::uint32_t>(0); // table entry size
                    ++index;
                }
            }
    };
    
}
namespace bbe::formats::elf{
    BBE_EXPORT ElfFile;
}
