//
// Created by djordy on 12/22/22.
//

#ifndef ASSEMBLER_TEMPLATES_H
#define ASSEMBLER_TEMPLATES_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include "magic_enum/magic_enum.hpp"

template<typename T>
std::map<std::string, T> enum_to_name_map() {
    std::map<std::string, T> map;
    auto pairs = magic_enum::enum_entries<T>();

    for (auto &r: pairs) {
        map.insert({std::string{r.second}, r.first});
    }

    return map;
}

template<typename T>
bool enum_name_lookup(const std::string &name, T &id) {
    static auto lookup_map = enum_to_name_map<T>();
    auto it = lookup_map.find(name);

    if (it == lookup_map.end()) {
        return false;
    }

    id = it->second;
    return true;
}

template<typename enum_t, typename value_t>
class enum_lut {
        std::array<value_t, (std::size_t) enum_t::LAST> lut;

public:
    using iterator = typename std::array<value_t, (std::size_t) enum_t::LAST>::iterator;

    enum_lut(std::vector<std::pair<enum_t, value_t>> lut_arg) {
        assert(lut_arg.size() == (std::size_t) enum_t::LAST);
        for (auto &p: lut_arg)
            lut[(uint) p.first] = p.second;
    };

    const iterator begin() const {
        return (iterator) lut.begin();
    };

    const iterator end()  const{
        return (iterator) lut.end();
    }


    const value_t &operator[](enum_t index) const {
        assert(index < enum_t::LAST);
        return lut[(uint) index];
    }

    const value_t &operator[](int index) const {
        assert(index < (int) enum_t::LAST);
        return lut[index];
    }
};

#endif //ASSEMBLER_TEMPLATES_H
