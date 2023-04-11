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
    auto ss = std::stringstream{};
    for (auto i = 0; i < nargs; i++) {
        ss << args[i] << ' ';
    }
    log<true, false>(ss.str());
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
    if (argc < 2) {
        log("error: no source file provided (repl not supported yet)");
        return 1;
    }
    auto s = read_text_file(fname);
    if (!s.has_value()) {
        log("error opening source file: " + std::string(fname));
        return 2;
    }
    auto& source = s.value();
    
    try {
        auto vm = Interpreter {};
        vm.set_global("print2", value_from_cfunction(tack_print));
        // vm.register_c_func("random", tack_random);
        // vm.register_c_func("clock", tack_clock);
        vm.execute(source, argc, argv);
    } catch (std::exception& e) {
        log(e.what());
    }

    return 0;
}
