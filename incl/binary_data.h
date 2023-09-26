//
// Created by djordy on 12/26/22.
//

#ifndef ASSEMBLER_BINARY_DATA_H
#define ASSEMBLER_BINARY_DATA_H
#include <inttypes.h>
#include <vector>

namespace binary_data {
    enum struct allignment_t { byte = 0, halfword = 1, word = 2 };

    struct memory_alloc_t {
        uint32_t nbytes = 0;
        allignment_t allignment = allignment_t::byte;
    };

    struct data_alloc_t {
        bool zero_data = true;
        memory_alloc_t memory_alloc;
        std::vector<int32_t> values;
    };

    inline uint32_t allign(uint32_t val, allignment_t allignment) {
        switch (allignment) {
            case allignment_t::byte:
                break;

            case allignment_t::halfword:
                val += val & 0b01;
                break;

            case allignment_t::word:
                val += val & 0b01;
                val += val & 0b10;
                break;
        }

        return val;
    }

    class alligned_counter {
        uint32_t count_m;

    public:
        alligned_counter(uint32_t count) : count_m(count){};
        alligned_counter() : alligned_counter(0){};

        uint32_t increment(memory_alloc_t size) {
            count_m = allign(count_m, size.allignment);
            uint32_t old_count = count_m;
            count_m += size.nbytes;
            return old_count;
        }

        uint32_t get_count() const {
            return count_m;
        }

        void reset() {
            count_m = 0;
        }
    };
}


#endif //ASSEMBLER_BINARY_DATA_H
