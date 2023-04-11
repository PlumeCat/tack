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
    return value_null();
}
Value tack_random(int nargs, Value* args) {
    return value_from_number(rand());
}
Value tack_clock(int nargs, Value* args) {
    return value_from_number(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1e6);
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
        vm.set_global("print", value_from_cfunction(tack_print));
        vm.set_global("random", value_from_cfunction(tack_random));
        vm.set_global("clock", value_from_cfunction(tack_clock));
        vm.execute(source, argc, argv);
    } catch (std::exception& e) {
        log(e.what());
    }

    return 0;
}
