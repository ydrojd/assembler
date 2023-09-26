//
// Created by djordy on 12/11/22.
//

#include "address_counter.h"
#include <stdexcept>

uint32_t address_counter::get_alligned(allignment_type alligned) {
    switch (alligned) {
        case allignment_type::byte: {
            return count_m;
        }

        case allignment_type::halfword: {
            return (count_m + (count_m % 2));
        }

        case allignment_type::fullword: {
            if(count_m % 4 == 0)
                return count_m;
            else
                return (4 - (count_m % 4) + count_m);
        }
    }

    return 0;
}

void address_counter::increment(allignment_type alligned, int nbytes) {
    uint32_t new_count;
    new_count = get_alligned(alligned) + nbytes;

    if(new_count < count_m) {
        throw std::overflow_error("address counter overflow");
    }

    count_m = new_count;
}