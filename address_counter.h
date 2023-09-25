//
// Created by djordy on 12/11/22.
//

#ifndef ASSEMBLER_ADDRESS_COUNTER_H
#define ASSEMBLER_ADDRESS_COUNTER_H

#include <stdint.h>

class address_counter {
    uint32_t count_m = 0;

public:
    enum struct allignment_type {
        byte,
        halfword,
        fullword
    };

    uint32_t get_alligned(allignment_type alligned);
    void increment(allignment_type, int nbytes);
    address_counter(uint32_t address) : count_m(address) {};
};


#endif //ASSEMBLER_ADDRESS_COUNTER_H
