#include"assemble.hpp"
#include<type_traits>
#include<bit>
namespace bbe::impl{
    struct Section{
        cppp::frozenstaticbuffer<8uz> name;
        std::uint32_t sflags;
        std::uint32_t begin;
        cppp::frozenbuffer data;
        std::uint32_t size = data.size();
    };
    struct PE{
        std::uint32_t file_alignment;
        std::uint32_t section_alignment;
        std::vector<Section> sections;
        std::uint32_t base_of_code;
        std::uint32_t entry_point;
        std::uint16_t nt_image_flags;
        std::uint16_t windows_extended_flags;
        std::vector<std::pair<std::uint16_t,std::uint16_t>> data_directories;
    };
    namespace ntf{
        constexpr inline std::uint16_t GOOD = 0x02;//file is valid
        constexpr inline std::uint16_t BIGADDR = 0x20;//supports >2G addresses
        constexpr inline std::uint16_t NODBGNFO = 0x200;//debug info is stripped
    }
    namespace winf{
        constexpr inline std::uint16_t ASLR = 0x20;//supports ASLR
        constexpr inline std::uint16_t RELOCATABLE = 0x40;//relocatable
        constexpr inline std::uint16_t FGMEMXPERM = 0x100;//supports fine-grained control of memory execution permissions
    }
    namespace secf{
        constexpr inline std::uint32_t TEXT = 0x20;
        constexpr inline std::uint32_t DATA = 0x40;
        constexpr inline std::uint32_t BSS = 0x80;
        constexpr inline std::uint32_t SHARE = 0x10000000;
        constexpr inline std::uint32_t EXEC = 0x20000000;
        constexpr inline std::uint32_t READ = 0x40000000;
        constexpr inline std::uint32_t WRITE = 0x80000000;
    }
    std::uint32_t imodcmpl(std::uint32_t p,std::uint32_t q){
        if(std::popcount(q)==1){
            return (-p)&(q-1);
        }
        return q-(p%q);
    }
    std::uint32_t iceil(std::uint32_t p,std::uint32_t q){
        return p+imodcmpl(p,q);
    }
    
    class OutputFile{
        const static std::uint16_t NT_AMD64_MAGIC = 0x8664;
        const static std::uint16_t COFF_64BIT_MAGIC = 0x20B;
        const static std::uint64_t SEC_HEAD_SIZE = 40;
        cppp::BinaryFile& bf;
        std::uint32_t where{0};//32-bit because windows doesn't support >4GB exe files.
        void seek(std::uint32_t pos){
            bf.seek(pos,std::ios_base::beg);
            where = pos;
        }
        void falign(std::uint32_t al){
            std::uint32_t fill = imodcmpl(where,al);
            where += fill;
            while(fill--){
                bf.writeb(0);
            }
        }
        void write(cppp::frozenbuffer buf){
            bf.write(buf);
            where += buf.size();
        }
        void writeq(std::uint64_t d){
            bf.writeqle(d);
            where += 8;
        }
        void writed(std::uint32_t d){
            bf.writedle(d);
            where += 4;
        }
        void writew(std::uint16_t d){
            bf.writewle(d);
            where += 2;
        }
        void writeb(std::uint8_t d){
            bf.writeb(d);
            ++where;
        }
        public:
            OutputFile(cppp::BinaryFile& f) : bf(f){}
            void write_section_header(const Section& sec,std::uint64_t section_start,std::uint32_t file_align){
                write(sec.name);
                writed(sec.size);//section size in memory
                writed(sec.begin);//start of section in memory
                writed(iceil(sec.data.size(),file_align));//section size in file
                writed(section_start);//start of section in file
                writed(0x00);//pointer to relocations; 0 for executable files
                writed(0x00);//start of COFF line number entries; 0 as it is deprecated
                writew(0x00);//number of relocations; 0 for executable files
                writew(0x00);//number of COFF line number entries; 0 as it is deprecated
                //section feature flags
                writed(sec.sflags);
            }
            void write_pe(const PE& pc){
                //DOS header
                writew(0x5A4D);//magic number
                //DOS metadata; zeroed out for simplicity (29w = 7q+1w)
                writeq(0);writeq(0);writeq(0);writeq(0);
                writeq(0);writeq(0);writeq(0);
                writew(0);
                //start of NT header
                writed(0x80);
                std::uint32_t end_of_dos_head = where;
                //DOS stub; zeroed out (64b = 8q)
                for(std::uint32_t i=0;i<8;++i){
                    writeq(0);
                }
                //NT header
                writed(0x4550);//signature
                writew(NT_AMD64_MAGIC);//architecture
                writew(pc.sections.size());//number of sections
                writed(0x00);//timestamp
                //start and length of symbol table; zeroed out because it's deprecated (2d = 1q)
                writeq(0);
                std::uint32_t opt_head_size_where = where;
                writew(0x00);//size of optional header
                //feature flags (characteristics)
                writew(pc.nt_image_flags);
                std::uint32_t start_of_opt_head = where;
                //COFF standard header
                writew(COFF_64BIT_MAGIC);//architecture
                writew(0x00);//linker version; zeroed out (2b = 1w)
                std::uint32_t text_size = 0;
                std::uint32_t data_size = 0;
                std::uint32_t bss_size = 0;
                for(const auto& sec : pc.sections){
                    if(sec.sflags&secf::TEXT){
                        text_size += sec.size;
                    }
                    if(sec.sflags&secf::DATA){
                        data_size += sec.size;
                    }
                    if(sec.sflags&secf::BSS){
                        bss_size += sec.size;
                    }
                }
                writed(text_size);
                writed(data_size);
                writed(bss_size);
                writed(pc.entry_point);//entry point
                writed(pc.base_of_code);//start of text in memory
                //Windows specific header
                writeq(0x10000);//preferred base; default = 0x10000
                writed(pc.section_alignment);//section alignment
                writed(pc.file_alignment);//file alignment
                //major+minor OS version, image version and subsystem version
                writew(0x05);writew(0x00);//OS
                writew(0x00);writew(0x00);//image
                writew(0x05);writew(0x02);//subsystem
                writed(0x00);//reserved = 0
                std::uint32_t imsize = 0;
                for(const auto& sec : pc.sections){
                    imsize = std::max(imsize,sec.begin+iceil(sec.size,pc.section_alignment));
                }
                writed(imsize);//image size rounded up to section alignment
                std::uint32_t headers_where = where;
                writed(0x00);//all headers size rounded up to file aligment
                writed(0x00);//checksum
                writew(0x03);//subsystem = console
                //more feature flags (characteristics)
                writew(pc.windows_extended_flags);
                writeq(0x2000);//max stack size
                writeq(0x800);//initial stack size
                writeq(0x100000);//max heap size
                writeq(0x200);//initial heap size
                writed(0x00);//reserved = 0
                writed(pc.data_directories.size());//number of data-directory entries
                for(const auto& directory : pc.data_directories){
                    writed(directory.first);
                    writed(directory.second);
                }
                {
                    std::uint32_t ptr = where;
                    seek(opt_head_size_where);
                    writew(ptr-start_of_opt_head);
                    seek(ptr);
                }
                cppp::fixed_array<std::uint64_t> sec_start{pc.sections.size()};
                {
                    std::uint32_t ptr = where+SEC_HEAD_SIZE*pc.sections.size();
                    //Section table
                    for(const auto& sec : pc.sections){
                        ptr += imodcmpl(ptr,pc.file_alignment);
                        write_section_header(sec,ptr,pc.file_alignment);
                        ptr += sec.data.size();
                    }
                }
                {
                    std::uint32_t ptr = where;
                    seek(headers_where);
                    writed(iceil(ptr-end_of_dos_head,pc.file_alignment));
                    seek(ptr);
                }
                falign(pc.file_alignment);
                for(const auto& sec : pc.sections){
                    write(sec.data);
                    falign(pc.file_alignment);
                }
            }
    };
    constexpr inline std::uint32_t FILE_ALIGN = 0x200;
    constexpr inline std::uint32_t SEC_ALIGN = 0x1000;
    constexpr inline std::uint32_t M_BASE = 0x1000;
    cppp::frozenstaticbuffer<8uz> operator""_secname(const char8_t* s,std::size_t){
        return std::as_bytes(std::span<const char8_t,8uz>{s,0uz});
    }
    void pushdw(std::vector<std::byte>& buf,std::uint32_t dw){
        buf.emplace_back(static_cast<std::byte>(dw));
        buf.emplace_back(static_cast<std::byte>(dw>>8));
        buf.emplace_back(static_cast<std::byte>(dw>>16));
        buf.emplace_back(static_cast<std::byte>(dw>>24));
    }
    void write_binary(cppp::BinaryFile& file,const BinaryInfo& bi){
        std::uint32_t rva = M_BASE;
        PE pe{
            .file_alignment = FILE_ALIGN,
            .section_alignment = SEC_ALIGN,
            .sections{},
            .base_of_code = rva,
            .entry_point = rva,
            .nt_image_flags = ntf::GOOD | ntf::BIGADDR,
            .windows_extended_flags = winf::ASLR | winf::RELOCATABLE | winf::FGMEMXPERM
        };
        pe.data_directories.resize(16,{0,0});
        if(bi.text.data()){
            pe.sections.emplace_back(Section{
                .name = u8".text\0\0\0"_secname,
                .sflags = secf::TEXT | secf::READ | secf::EXEC,
                .begin = rva,
                .data = bi.text
            });
            rva += bi.text.size();
            rva = iceil(rva,SEC_ALIGN);
        }
        if(bi.data.data()){
            pe.sections.emplace_back(Section{
                .name = u8".data\0\0\0"_secname,
                .sflags = secf::DATA | secf::READ | secf::WRITE,
                .begin = rva,
                .data = bi.data
            });
            rva += bi.data.size();
            rva = iceil(rva,SEC_ALIGN);
        }
        std::vector<std::byte> idata;
        {
            std::uint32_t ilt = rva+(bi.imports.size()+1)*20;
            std::uint32_t dll_names = ilt+(bi.imports.size()+1)*8;
            std::uint32_t dll_name_p = dll_names;
            for(const auto& import : bi.imports){
                pushdw(idata,ilt);//address of ILT entry
                pushdw(idata,0);//timestamp
                pushdw(idata,0);//forwarder chain
                pushdw(idata,dll_name_p);//referenced dll name
                pushdw(idata,ilt);//address of IAT entry
                rva += 20;
                ilt += 8;
                dll_name_p += import.name.size()+1;
            }
            idata.resize(idata.size()+20uz);//NULL IDT entry
            rva += 20;
            for(const auto& import : bi.imports){
                pushdw(idata,hint_name_table);
                pushdw(idata,0);
            }
            for(const auto& import : bi.imports){
                idata.insert(idata.end(),import.name.begin(),import.name.end());
                idata.emplace_back(0);
                rva += import.name.size()+1;
            }
            pe.sections.emplace_back(Section{
                .name = u8".idata\0\0"_secname,
                .sflags = secf::DATA | secf::READ,
                .begin = rva,
                .data = std::as_bytes(std::span<const std::uint32_t>(idata))
            });
            pe.data_directories[1uz].first = rva;
            pe.data_directories[1uz].second = bi.imports.size()*20;
            rva = iceil(rva,SEC_ALIGN);
        }
        OutputFile(file).write_pe(pe);
    }
}
