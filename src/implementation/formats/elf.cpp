#include<bbe/formats/elf.hpp>
namespace bbe::formats::elf::impl{
    void Elf::add_text(const targets::x64::X64Program& prog){
        using namespace std::literals::string_view_literals;
        using enum targets::x64::SymbolType;
        add_section(u8".text"sv,1/*binary data*/,prog.text().data(),prog.text().size(),0x1000,0x10,true,false,true);
        {
            cppp::bytes& symtab = section_data.emplace_back();
            symtab.resize(0x18);
            for(const targets::x64::SymbolInfo& sym : prog.symbols()){
                symtab.appendl<std::uint32_t>(symbol_names.add(sym.name));
                symtab.append(
                    (1 << 4) | // HI 1: global binding / linkage
                    (
                        sym.type==FUNCTION?2 // code
                        :sym.type==VARIABLE?1 // object
                        :throw std::logic_error("Elf::add_text(): Error creating symbol table: unknown symbol type")
                    )
                );
                symtab.append(0); // default visibility (allow interposing)
                if(sym.defined_begin==sym.b_import_only){
                    symtab.appendl<std::uint16_t>(0);
                    symtab.appendl<std::uint64_t>(0);
                }else{
                    symtab.appendl(static_cast<std::uint16_t>(sections.size())); // linked section ID (to .text; adding after registering .text therefore +1, because of null section)
                    symtab.appendl<std::uint64_t>(sym.defined_begin);
                }
                symtab.appendl<std::uint64_t>(0);
            }
            add_section(u8".symtab"sv,2/*symbol table*/,symtab.data(),symtab.size(),0,0,false,false,false,0x18,
            static_cast<std::uint32_t>(SYMBOL_NAME_TABLE_INDEX+1),1);
        }
        if(!prog.relocations().empty()){
            cppp::bytes& reltab = section_data.emplace_back();
            for(const targets::x64::Relocation& rel : prog.relocations()){
                reltab.appendl<std::uint64_t>(rel.offset);
                reltab.appendl<std::uint64_t>((rel.symbol+1)<<32uz | 2 /* PC-relative */);
                reltab.appendl<std::uint64_t>(-rel.isize);
            }
            add_section(u8".rela.text"sv,4/*relocations table*/,reltab.data(),reltab.size(),0,0,false,false,false,0x18,
static_cast<std::uint32_t>(sections.size()),static_cast<std::uint32_t>(sections.size()-1));
        }
    }
    cppp::bytes Elf::encode() const{
        sections[SECTION_NAME_TABLE_INDEX].data = section_names.data();
        sections[SECTION_NAME_TABLE_INDEX].size = section_names.size();
        sections[SYMBOL_NAME_TABLE_INDEX].data = symbol_names.data();
        sections[SYMBOL_NAME_TABLE_INDEX].size = symbol_names.size();
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
        constexpr std::size_t SHT_ENTRY_SIZE = 0x40uz;
        data.appendl(static_cast<std::uint16_t>(SHT_ENTRY_SIZE));
        data.appendl(static_cast<std::uint16_t>(sections.size()+1)); // SHT entry count (+ null entry)
        data.appendl<std::uint16_t>(SECTION_NAME_TABLE_INDEX+1); // + null entry
        // section data
        std::vector<std::uint64_t> sdaddr;
        for(const Section& sec : sections){
            sdaddr.emplace_back(data.size());
            if(sec.data){
                data.append({sec.data,sec.size});
            }
        }
        // SHT
        data.writel(SHT_OFFSET_POS,static_cast<std::uint64_t>(data.size()));
        data.resize(data.size()+SHT_ENTRY_SIZE); // null entry
        std::size_t index = 0uz;
        for(const Section& sec : sections){
            data.appendl<std::uint32_t>(sec.name); // name index
            data.appendl<std::uint32_t>(sec.type); // content type
            std::uint64_t flags{0};
            if(sec.load){
                flags |= 2; // loaded in memory
            }
            if(sec.write){
                flags |= 1; // runtime writable
            }else if(sec.exec){
                flags |= 4; // machine code
            }
            data.appendl<std::uint64_t>(flags);
            data.appendl<std::uint64_t>(sec.m_addr); // loaded memory address
            data.appendl<std::uint64_t>(sdaddr[index]); // file address
            data.appendl(static_cast<std::uint64_t>(sec.size)); // size
            data.appendl<std::uint32_t>(sec.link); // link
            data.appendl<std::uint32_t>(sec.info); // info
            data.appendl<std::uint64_t>(sec.align); // alignment
            data.appendl<std::uint64_t>(sec.entsize); // table entry size
            ++index;
        }
        return data;
    }
}
