//
// Created by djordy on 12/25/22.
//
#include "elfio/elfio.hpp"
#include "binary_data.h"
#include "symbol_table.h"
#include "isa.h"

#include "elf_generator.h"
void elf_generator::initialise() {
    //set 32 bit and little endian 2s compliment
    writer.create(ELFIO::ELFCLASS32, ELFIO::ELFDATA2LSB);

    //not a known platform
    writer.set_os_abi(ELFIO::ELFOSABI_STANDALONE);
    writer.set_machine(ELFIO::EM_NONE);

    //executable elf
    writer.set_type(ELFIO::ET_EXEC);

    /*---text section--*/
    text_sec = writer.sections.add(".text");
    text_sec->set_type(ELFIO::SHT_PROGBITS);//set section type to program data
    text_sec->set_flags(
            ELFIO::SHF_EXECINSTR |//executable section
            ELFIO::SHF_ALLOC);//section needs to be allocated in memory during execution

    text_sec->set_addr_align(4);//set allignment to 4 bytes (word)

    /*---data section--*/
    data_sec = writer.sections.add(".data");
    data_sec->set_type(ELFIO::SHT_PROGBITS);//set section type to program data
    data_sec->set_flags(
            ELFIO::SHF_WRITE//writable data
            |
            ELFIO::SHF_ALLOC);//section needs to be allocated in memory during execution
    data_sec->set_addr_align(4);//set allignment to 4 bytes (word)

    /*---bss section---*/
    bss_sec = writer.sections.add(".bss");
    bss_sec->set_type(ELFIO::SHT_NOBITS);// Program space with no data
    bss_sec->set_flags(
            ELFIO::SHF_WRITE |//writable data
            ELFIO::SHF_ALLOC);//section needs to be allocated in memory during execution
    bss_sec->set_addr_align(4);//set allignment to 4 bytes (word)


    /*---rodata section--*/
    rodata_sec = writer.sections.add(".rodata");
    rodata_sec->set_type(ELFIO::SHT_PROGBITS);//set section type to program data
    rodata_sec->set_flags(
            ELFIO::SHF_ALLOC);//section needs to be allocated in memory during execution
    rodata_sec->set_addr_align(4);//set allignment to 4 bytes (word)


    //---string section---*/
    str_sec = writer.sections.add(".strtab");
    str_sec->set_type(ELFIO::SHT_STRTAB);

    //---symbtab section---//
    sym_sec = writer.sections.add(".symtab");
    sym_sec->set_type(ELFIO::SHT_SYMTAB); //set symbol table type
    sym_sec->set_addr_align( 0x4 ); //set allignment to 4 bytes (word)
    sym_sec->set_entry_size(writer.get_default_entry_size( ELFIO::SHT_SYMTAB ) );
    sym_sec->set_link( str_sec->get_index() ); //link to the string table

    //---reloc section--//
    // for symbolic references
    rel_sec = writer.sections.add(".rel"); //relocates for the text section
    rel_sec->set_type(ELFIO::SHT_REL); //relocate table type
    rel_sec->set_info(text_sec->get_index()); //in context of text section
    rel_sec->set_addr_align( 0x4 ); //set allignment to 4 bytes (word)
    rel_sec->set_entry_size(writer.get_default_entry_size( ELFIO::SHT_REL));
    rel_sec->set_link( sym_sec->get_index() ); //link to symbol table section
}

void elf_generator::set_data_section() {
    working_section = section_t::data;
}

void elf_generator::set_text_section() {
    working_section = section_t::text;
}

void elf_generator::set_bss_section() {
    working_section = section_t::bss;
}

void elf_generator::set_rodata_section() {
    working_section = section_t::rodata;
}

std::string &elf_generator::get_working_data() {
    switch (working_section) {
        case section_t::text:
            return text;
        case section_t::rodata:
            return rodata;
        case section_t::data:
            return data;
        case section_t::bss:
            return bss;
        case section_t::none:
            assert(false);
    }
}

uint32_t elf_generator::push_instruction(const isa::instruction &inst) {
    auto encoded = inst.encode();

    uint32_t address;

    if (encoded.size == isa::instruction_size_type::halfword) {
        address = push_data(static_cast<uint16_t>(encoded.halfword));
    } else if (encoded.size == isa::instruction_size_type::fullword) {
        address = push_data(static_cast<uint32_t>(encoded.fullword));
    }

    return address;
}

void elf_generator::write() {
    //load text section
    text_sec->set_data(text.c_str(), text.size()); //load text section data

    //load rodata section
    rodata_sec->set_data(rodata.c_str(), rodata.size());

    //load data section
    data_sec->set_data(data.c_str(), data.size());

    //load bss section
    bss_sec->set_size(bss.size());

    //---write file---//
    writer.set_entry(entry_point);
    writer.save(output_fname);
}

uint32_t elf_generator::push_data(const binary_data::data_alloc_t &data_alloc) {
    if(data_alloc.zero_data)
        return push_zero_alloc(data_alloc.memory_alloc);

    auto &working_data = get_working_data();

    //---align data---//
    const auto placement_address = allign_to(data_alloc.memory_alloc.allignment);

    //---insert data--//
    switch (data_alloc.memory_alloc.allignment) {
        case binary_data::allignment_t::word:
            for (auto &val : data_alloc.values)
                insert_data(static_cast<uint32_t>(val));

            break;
        case binary_data::allignment_t::halfword:
            for (auto &val : data_alloc.values)
                insert_data(static_cast<uint16_t>(val));

            break;
        case binary_data::allignment_t::byte:
            for (auto &val : data_alloc.values)
                insert_data(static_cast<uint8_t>(val));

            break;
    }

    //returns address data was placed at
    return placement_address;
}

uint32_t elf_generator::allign_to(binary_data::allignment_t allignment) {
    auto &working_data = get_working_data();
    const uint32_t alligned_size = binary_data::allign(working_data.size(), allignment);
    const uint32_t diff = alligned_size - working_data.size();
    working_data.insert(working_data.size(), diff, 0);
    return alligned_size;
}

void elf_generator::insert_data(uint8_t val) {
    get_working_data().push_back(val);
}

void elf_generator::insert_data(uint16_t val) {
    //least significant first
    insert_data(static_cast<uint8_t>(val));
    //most significant second
    insert_data(static_cast<uint8_t>(val >> 8));
}

void elf_generator::insert_data(uint32_t val) {
    //least significant first
    insert_data(static_cast<uint16_t>(val));
    //most significant second
    insert_data(static_cast<uint16_t>(val >> 16));
}

uint32_t elf_generator::push_data(uint32_t val) {
    const auto placement_address = allign_to(binary_data::allignment_t::word);
    insert_data(val);
    return placement_address;
}

uint32_t elf_generator::push_data(uint16_t val) {
    const auto placement_address = allign_to(binary_data::allignment_t::halfword);
    insert_data(val);
    return placement_address;
}

uint32_t elf_generator::push_data(uint8_t val) {
    const auto placement_address = get_working_data().size();
    insert_data(val);
    return placement_address;
}

uint32_t
elf_generator::push_zero_alloc(const binary_data::memory_alloc_t &mem_alloc) {
    uint32_t placement_address = 0;
    //align
    placement_address = allign_to(mem_alloc.allignment);
    //insert zeros
    get_working_data().insert(get_working_data().size(), mem_alloc.nbytes, 0);

    return placement_address;
}

uint32_t elf_generator::add_string(const std::string &str) {
    ELFIO::string_section_accessor str_writer(str_sec);
    return str_writer.add_string(str);
}

uint32_t elf_generator::insert_symbol_def(const symbol &sym) {
    ELFIO::symbol_section_accessor sym_writer(writer, sym_sec);

    assert(sym.identifier.size() != 0);

    const ELFIO::Elf32_Word name = add_string(sym.identifier);
    const ELFIO::Elf32_Addr address_value = sym.address;
    const ELFIO::Elf32_Word size = sym.size;
    unsigned char info;

    //---determine info (type)---//
    switch (sym.type) {
        case symbol_type::data:
            info = ELFIO::STT_OBJECT;
            break;

        case symbol_type::fun:
            info = ELFIO::STT_FUNC;
            break;

        case symbol_type::undefined:
            info = ELFIO::STT_NOTYPE;
            break;
        default:
            assert(false);
    }

    //---determine info (visibility)
    switch (sym.scope) {
        case symbol_scope::local:
            info |= ELFIO::STB_LOCAL << 4;
            break;

        case symbol_scope::global:
        case symbol_scope::external:
            info |= ELFIO::STB_GLOBAL << 4;
            break;
    }

    //---determine index---//
    ELFIO::Elf32_Word section_index;
    switch (sym.scope) {
        case symbol_scope::global:
        case symbol_scope::local:
            section_index = get_sec_index(sym.section);
            break;

        case symbol_scope::external:
            section_index = 0;
            break;
    }


    return sym_writer.add_symbol(name, address_value, size, info, 0, section_index);
}

ELFIO::Elf_Half elf_generator::get_sec_index(binary::section_t section){
    switch (section) {
        case binary::section_t::text:
            return text_sec->get_index();
        case binary::section_t::data:
            return data_sec->get_index();
        case binary::section_t::bss:
            return bss_sec->get_index();
        case binary::section_t::rodata:
            return rodata_sec->get_index();
    }

    assert(false);
}

void elf_generator::insert_symbol_ref(const symbol_ref &sref) {
    ELFIO::relocation_section_accessor ref_writer(writer, rel_sec);
    const ELFIO::Elf32_Addr address_value = sref.address;
    const ELFIO::Elf_Word symbol_index = sref.symbol_id;
    ref_writer.add_entry(address_value, symbol_index, (int) sref.type);
}
void elf_generator::set_entrypoint(uint32_t address) {
    entry_point = address;
}
