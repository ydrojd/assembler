//
// Created by djordy on 12/25/22.
//

#ifndef ASSEMBLER_ELF_GENERATOR_H
#define ASSEMBLER_ELF_GENERATOR_H

#include "elfio/elfio.hpp"
#include "binary_data.h"
#include "symbol_table.h"
#include "isa.h"

class elf_generator {

public:
    enum struct section_t {
        text,
        rodata,
        data,
        bss,
        none,
    };

private:
    const std::string output_fname;
    ELFIO::elfio writer;

    ELFIO::section *text_sec;
    ELFIO::section *rodata_sec;
    ELFIO::section *data_sec;
    ELFIO::section *bss_sec;

    ELFIO::section *str_sec;
    ELFIO::section *sym_sec;
    ELFIO::section *rel_sec;

    std::string text;
    std::string rodata;
    std::string data;
    std::string bss;
    section_t working_section = section_t::none;
    std::string &get_working_data();

    uint32_t entry_point = 0;

    void initialise();
    uint32_t allign_to(binary_data::allignment_t allignment);
    void insert_data(uint8_t val);
    void insert_data(uint16_t val);
    void insert_data(uint32_t val);

    ELFIO::Elf_Half get_sec_index(binary::section_t section);
public:
    elf_generator(const std::string &fname) : output_fname(fname) {initialise(); }
    void set_entrypoint(uint32_t address);

    uint32_t push_instruction(const isa::instruction &inst);
    uint32_t push_data(const binary_data::data_alloc_t &data_alloc);
    uint32_t push_data(uint8_t val);
    uint32_t push_data(uint16_t val);
    uint32_t push_data(uint32_t val);
    uint32_t push_zero_alloc(const binary_data::memory_alloc_t &mem_alloc);

    void set_text_section();
    void set_data_section();
    void set_bss_section();
    void set_rodata_section();

    uint32_t add_string(const std::string &str);
    uint32_t insert_symbol_def(const symbol &sym);
    void insert_symbol_ref(const symbol_ref &sref);

    void write();
};


#endif//ASSEMBLER_ELF_GENERATOR_H
