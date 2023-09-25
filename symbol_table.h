//
// Created by djordy on 12/11/22.
//

#ifndef ASSEMBLER_SYMBOL_TABLE_H
#define ASSEMBLER_SYMBOL_TABLE_H

#include <vector>
#include <inttypes.h>
#include <string>
#include "binary.h"
#include <map>
#include <assert.h>

enum struct symbol_scope {
    local,
    global,
    external,
};

enum struct symbol_type {
    fun,
    data,
    undefined
};

struct symbol {
    binary::section_t section = binary::section_t::undefined;
    std::string identifier = "undefined";
    uint32_t address = 0;
    symbol_type type = symbol_type::data;
    symbol_scope scope = symbol_scope::local;
    uint32_t size = 0;
};

struct symbol_ref {
    int symbol_id = 0;
    uint32_t address = 0;
    binary::reloc_type type;
};

class symbol_table {
    std::map<std::string, int32_t> identifer_to_id;
    std::vector<symbol> symbols;
    std::vector<symbol_ref> refs;

public:
    symbol_table() {
        symbols.push_back({}); //a first undefined symbol has to be inserted
                                //this will cause the symbol id's to be alligned with those returned from .symtab section in elf file binary generetor,
    }

    int32_t get_id(const std::string &identifier, bool &found) const {
        const auto it = identifer_to_id.find(identifier);
        if (it == identifer_to_id.end()) {
            found = false;
            return 0;
        }

        found = true;
        return it->second;
    }

    const symbol &get_symbol(int32_t id) const {
        assert(id < symbols.size());
        return symbols[id];
    }

    void insert_ref(symbol_ref ref) {
        assert(ref.symbol_id < symbols.size());
        refs.push_back(ref);
    }

    int32_t insert_symbol(const symbol &sym, bool &success) {
        bool has_identifier;
        get_id(sym.identifier, has_identifier);

        if (has_identifier == true) {
            success = false;
            return 0;
        }

        int32_t id = symbols.size();
        identifer_to_id.emplace(sym.identifier, id);
        symbols.push_back(sym);
        success = true;
        return id;
    }

    void update_symbol_address(int32_t id, uint32_t address) {
        assert(id < symbols.size());
        symbols[id].address = address;
    }

    using iterator = std::vector<symbol>::iterator;

    iterator begin() {
        return (iterator) (symbols.begin() + 1);
    }

    iterator end() {
        return (iterator) symbols.end();
    }

    const std::vector<symbol_ref> &get_ref_data() const {
        return refs;
    }

    symbol &operator[](int id) {
        assert(id < symbols.size());
        return symbols[id];
    }

    const symbol &operator[](int id) const {
        assert(id < symbols.size());
        return symbols[id];
    }

};

#endif //ASSEMBLER_SYMBOL_TABLE_H
