//
// Created by djordy on 12/29/22.
//

#ifndef ASSEMBLER_BINARY_H
#define ASSEMBLER_BINARY_H

namespace binary {
enum struct section_t {
    text,
    data,
    bss,
    rodata,
    undefined,
    LAST
};

enum struct reloc_type {
    none = 0,
    symr_long_store = 1,
    symr_long_load = 2,
    secr_long_store = 3,
    secr_long_load = 4,
    short_jump = 5,
    long_jump = 6,
    dummy = 7
};

}

#endif//ASSEMBLER_BINARY_H
