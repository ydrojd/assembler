//
// Created by djordy on 12/22/22.
//

#ifndef ASSEMBLER_BITWISE_FUNCTIONS_H
#define ASSEMBLER_BITWISE_FUNCTIONS_H

#include <stdint.h>
#include <assert.h>
inline bool is_alligned(uint32_t val, uint8_t bits) {
    return (val & ((1 << bits) - 1)) == 0;
}

inline uint32_t allign (uint32_t val, uint8_t bits) {
    const bool not_alligned = !is_alligned(val, bits);
    const uint32_t to_add = (static_cast<uint32_t>(not_alligned) << bits);

    return (val & ~((1 << bits) - 1)) + to_add;
}

inline void bitwise_place(uint32_t &result, uint32_t val, uint8_t position, uint8_t size) {
    result = (result & ~(((1 << size) - 1) << position)) | (val << position);
}

inline void bitwise_place(uint16_t &result, uint32_t val, uint8_t position, uint8_t size) {
    result = (result & ~(((1 << size) - 1) << position)) | (val << position);
}

inline uint32_t bitwize_select(uint32_t val, uint8_t position, uint8_t size) {
    return (val >> position) & ((1 << size) - 1);
}

inline int32_t bitwize_select(int32_t val, uint8_t position, uint8_t size) {
    return (val >> position) & ((1 << size) - 1);
}

inline uint32_t bit_extend(uint32_t val, bool extend_bit, uint8_t position) {
    uint32_t bit_fence = (-1 << position);
    return (extend_bit) ? (val | bit_fence ) : (val & ~bit_fence);
}

inline int32_t signed_bit_extend(int32_t val, uint8_t bit_pos) {
    assert(bit_pos != 0);
    uint8_t bit = bitwize_select(val, bit_pos - 1, 1);
    bool bit_true = bit;
    return (bit_extend(val, bit_true, bit_pos));
}

#endif //ASSEMBLER_BITWISE_FUNCTIONS_H
