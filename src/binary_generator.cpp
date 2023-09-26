//
// Created by djordy on 1/2/23.
//

#include "binary_generator.h"
#include "compilation_unit_t.h"
#include "elf_generator.h"

void binary_generator(const std::string &fname, compilation_unit &comp_unit) {

    elf_generator elf(fname);

    //---insert instructions---//
    elf.set_text_section();
    for (auto &inst: comp_unit.instructions) elf.push_instruction(inst);

    //---insert data sections---//
    elf.set_data_section();
    for (auto &data_alloc: comp_unit.data) elf.push_data(data_alloc);

    elf.set_rodata_section();
    for (auto &data_alloc: comp_unit.rodata) elf.push_data(data_alloc);

    elf.set_bss_section();
    for (auto &data_alloc: comp_unit.bss)
        elf.push_zero_alloc(data_alloc.memory_alloc);

    //---insert symbols---//
    for (auto &sym : comp_unit.st) elf.insert_symbol_def(sym);

    //---insert relocs---//
    for (auto &sym_ref: comp_unit.st.get_ref_data())
        elf.insert_symbol_ref(sym_ref);

    //---set entry point---//
    bool found_symbol = false;
    const auto sym_id = comp_unit.st.get_id("start", found_symbol);
    if (found_symbol == true && sym_id != 0)
        elf.set_entrypoint(comp_unit.st[sym_id].address);
    else
        elf.set_entrypoint(0);

    elf.write();
}