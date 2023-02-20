#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <unordered_map>
#include <chrono>

#define JLIB_IMPLEMENTATION

#include <jlib/log.h>
#include <jlib/text_file.h>

using namespace std;
using namespace std::chrono;

enum class Type {
    Null,
    Boolean,
    Integer,
    Number,
    String,

    //Pointer,
    //Vector,
    //Matrix,
    Array,
    Object,
    Function
};

double nan_pack_int(uint32_t i) {
    // TODO: endianness
    auto top_16 = (uint64_t(0b0111111111111111)) << 48;
    auto _i = uint64_t(i) | top_16; // bottom bits not all 0
    auto d = *reinterpret_cast<double*>(&_i);
    return d;
}
uint32_t nan_unpack_int(double d) {
    auto i = *reinterpret_cast<uint64_t*>(&d);
    return i & 0xffffffff;
}


#include "parsing.h"
#include "ref_interpreter.h"
#include "bytecode_interpreter.h"


int main(int argc, char* argv[]) {
    auto check_arg = [&](const string& s) {
        for (auto i = 0; i < argc; i++) {
            if (argv[i] == s) {
                return true;
            }
        }
        return false;
    };
    std::ios::sync_with_stdio(false);
    auto fname = (argc >= 2) ? argv[1] : "source.rs";
    auto s = read_text_file(fname);
    if (!s.has_value()) {
        throw runtime_error("error opening source file: "s + argv[1]);
    }
    auto source = s.value();
    auto out_ast = AstNode {};
    try {
        parse(source, out_ast);
        try {
            if (check_arg("-A")) {
                log("AST: ", out_ast.tostring());
            }

            if (check_arg("--twi")) {
                log("Running with reference TWI");
                auto vm = RefInterpreter {};
                auto before = steady_clock::now();
                auto e = vm.eval(out_ast);
                auto after = steady_clock::now();
                log("Evaling", e);
                log("TWI done in ", duration_cast<microseconds>(after - before).count() * 1e-6, "s");
            }
            
            log("Compiling");
            auto vm2 = BytecodeInterpreter();
            auto program = Program {};
            auto compiler = Compiler {};
            compiler.compile(out_ast, program);
            if (check_arg("-D")) {
                log(program.to_string());
            }

            auto before = steady_clock::now();
            vm2.execute(program);
            auto after = steady_clock::now();
            log("BC done in ", duration_cast<microseconds>(after - before).count() * 1e-6, "s");
        } catch (exception& e) {
            log(e.what());
        }
    } catch (exception& e) {
        log("parser error: ", e.what());
    }

    if (argc < 2) {
        getchar();
    }

    return 0;
}
