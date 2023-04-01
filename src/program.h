#pragma once

#include <vector>
#include <string>
#include <sstream>

#include "instructions.h"
#include "value.h"

using std::string, std::vector, std::stringstream;

// struct Program {
    //vector<Instruction> instructions;
    //vector<Value> storage; // program constant storage goes at the bottom of the stack for now
    //list<string> strings; // string constants and literals storage - includes identifiers for objects
    //list<FunctionType> functions;

    // FunctionContext* root_context;


    //string to_string() {
    //    auto s = stringstream {};

    //    s << "Program:\n";
    //    s << "   Bytecode:\n";
    //    auto i = 0;
    //    for (auto bc : instructions) {
    //        s << "        " << i << ": " << ::to_string(bc.opcode) << ' ' << (uint32_t)bc.r0 << ' ' << (uint32_t)bc.r1 << ' ' << (uint32_t)bc.r2 << '\n';
    //        i++;
    //    }

    //    s << "    Storage:\n";
    //    i = 0;
    //    for (auto& x : storage) {
    //        s << "        " << i << ": " << x << '\n';
    //        i++;
    //    }

    //    s << "    Strings:\n";
    //    i = 0;
    //    for (auto& _s: strings) {
    //        s << "        " << i << ": " << _s << '\n';
    //        i++;
    //    }

    //    return s.str();
    //}

    //uint32_t add_string(const string& s) {
    //    strings.emplace_back(s);
    //    return strings.size() - 1;
    //}
    //uint32_t add_storage(const Value& value) {
    //    storage.emplace_back(value);
    //    return storage.size() - 1;
    //}

    //Value get_storage(uint32_t index) {
    //    return storage[index];
    //}
// };