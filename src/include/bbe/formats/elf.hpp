#pragma once
#include"commons.hpp"
#include<cppp/bytearray.hpp>
#include<cppp/string.hpp>
#include<vector>
#include<span>
namespace bbe::formats::elf::impl{
    class ElfFile{
        struct Section{
            cppp::str name;
            std::byte* data; // std::span(nullptr,nonzero_size) is UB.
            std::size_t size;
            std::uint64_t m_addr;
            std::uint64_t align;
            bool write;
            bool exec;
        };
        bool wide; // 64-bit if true, else 32-bit
        std::uint64_t entry_point;
        std::vector<Section> sections;
        public:
            ElfFile(bool wide=true) : wide(wide){}
            void add_section(cppp::str&& name,std::byte* data,std::size_t size,std::uint64_t m_addr,std::uint64_t align,bool writable,bool executable){
                sections.emplace_back(std::move(name),data,size,m_addr,align,writable,executable);
            }
            cppp::bytes encode() const{
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
                std::size_t name_table_begin = data.size();
                // name table
                data.append(std::byte{0});
                std::size_t name_table_size = 1; // must begin with zero
                data.append(as_bytes(std::span<const char8_t>(u8".shstrtab",10)));
                for(const Section& sec : sections){
                    nameindex.emplace_back(name_table_size);
                    std::size_t buffer_length = sec.name.length()+1; // +1 for null terminator
                    name_table_size += buffer_length;
                    data.append(as_bytes(std::span<const char8_t>(sec.name.c_str(),buffer_length)));
                }
                // sections
                for(const Section& sec : sections){
                    sdaddr.emplace_back(data.size());
                    if(sec.data){
                        data.append({sec.data,sec.size});
                    }
                }
                // SHT
                // name table
                data.appendl<std::uint32_t>(1); // name index
                data.appendl<std::uint32_t>(3); // content type, 3 = string table
                data.appendl<std::uint64_t>(0); // flags
                data.appendl<std::uint64_t>(0); // loaded memory address
                data.appendl(static_cast<std::uint64_t>(name_table_begin)); // file address
                data.appendl(static_cast<std::uint64_t>(name_table_size)); // size
                data.appendl<std::uint32_t>(0); // link
                data.appendl<std::uint32_t>(0); // info
                data.appendl<std::uint64_t>(0); // align
                data.appendl<std::uint64_t>(0); // table entry size
                
                // sections
                std::size_t index = 0uz;
                for(const Section& sec : sections){
                    data.appendl(static_cast<std::uint32_t>(nameindex[index])); // name index
                    data.appendl<std::uint32_t>(1); // content type, 1 = binary data
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
