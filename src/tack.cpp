#include "parsing.h"
#include "value.h"
#include "compiler.h"
#include "interpreter.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <unordered_map>
#include <chrono>
#include <filesystem>

#define JLIB_IMPLEMENTATION
#if (defined _MSC_VER && defined _DEBUG)
#define JLIB_LOG_VISUALSTUDIO
#endif
#include <jlib/log.h>
#include <jlib/text_file.h>

Value tack_print(int nargs, Value* args) {
    return { 0 };
}
Value tack_random(int nargs, Value* args) {
    return { 0 };
}
Value tack_clock(int nargs, Value* args) {
    return { 0 };
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    auto fname = (argc >= 2) ? argv[1] : "source.str";
    auto s = read_text_file(fname);
    if (!s.has_value()) {
        throw std::runtime_error("error opening source file: " + std::string(argv[1]));
    }
    auto& source = s.value();
    
    try {
        auto vm = Interpreter {};
        // vm.register_c_func("print", tack_print);
        // vm.register_c_func("random", tack_random);
        // vm.register_c_func("clock", tack_clock);
        vm.execute(source, argc, argv);
    } catch (std::exception& e) {
        log(e.what());
    }

    return 0;
}
