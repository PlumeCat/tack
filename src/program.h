#pragma once

#include <vector>
#include <string>
#include <sstream>

#include "instructions.h"
#include "value.h"

using std::string, std::vector, std::stringstream;

struct CaptureInfo {
    uint8_t source_register;
    uint8_t dest_register;
};
struct CompiledFunction {
    string name;
    vector<Instruction> instructions;
    vector<Value> storage; // program constant storage goes at the bottom of the stack for now
    list<string> strings; // string constants and literals storage - includes identifiers for objects
    list<CompiledFunction> functions; // local functions (everything is a local function from the POV of the global context)
    vector<CaptureInfo> capture_info;

    uint16_t store_number(double d) {
        storage.emplace_back(value_from_number(d));
        return storage.size() - 1;
    }

    uint16_t store_string(const string& data) {
        strings.emplace_back(data);
        storage.emplace_back(value_from_string(&strings.back()));
        return storage.size() - 1;
    }
    uint16_t store_function() {
        functions.emplace_back();
        storage.emplace_back(value_from_pointer(&functions.back()));
        return storage.size() - 1;
    }

    string str(const string& prefix = ""s) {
        auto s = stringstream {};
        s << "function: " << prefix + name << endl;
        auto i = 0;
        for (auto bc : instructions) {
            s << "    " << i << ": " << ::to_string(bc.opcode) << ' ' << (uint32_t)bc.r0 << ' ' << (uint32_t)bc.r1 << ' ' << (uint32_t)bc.r2 << '\n';
            i++;
        }

        s << "  storage:\n";
        i = 0;
        for (auto& x : storage) {
            s << "    " << i << ": " << x << '\n';
            i++;
        }

        s << "  strings:\n";
        i = 0;
        for (auto& _s : strings) {
            s << "    " << i << ": " << _s << '\n';
            i++;
        }

        for (auto& f : functions) {
            s << f.str(prefix + name + "::");
        }
        return s.str();
    }
};