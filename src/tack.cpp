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

Interpreter* vm;

Value tack_pong(int nargs, Value* args) {
    // auto func = value_to_function(args[0]);
    log("pong");
    vm->call(args[0], nullptr, 0);
}

int main(int argc, char* argv[]) {
    auto log_ast = false;
    auto log_bytecode = false;
    auto files = std::vector<std::string>{};
    for (auto i = 1; i < argc; i++) {
        auto arg = std::string(argv[i]);
        if (arg == "-A") {
            log_ast = true;
        } else if  (arg == "-D") {
            log_bytecode = true;
        } else {
            files.emplace_back(std::move(arg));
        }
    }

    std::ios::sync_with_stdio(false);
    if (!files.size()) {
        log("error: no source file provided (repl not supported yet)");
        return 1;
    }

    try {
        auto vm = Interpreter {};
        ::vm = &vm;
        vm.set_global("print", true, value_from_cfunction(tack_print));
        vm.set_global("random", true, value_from_cfunction(tack_random));
        vm.set_global("clock", true, value_from_cfunction(tack_clock));
        vm.set_global("pong", true, value_from_cfunction(tack_pong));

        for (auto& f: files) {
            log("executing source file:", f);
            auto s = read_text_file(f);
            if (!s.has_value()) {
                log("error opening source file:", f);
                return 2;
            }
            auto& source = s.value();
            vm.call(vm.load(source), nullptr, 0);
            
            auto foo = vm.get_global("foo");
            if (value_is_function(foo)) {
                vm.call(foo, nullptr, 0);
            }
        }
    } catch (std::exception& e) {
        log(e.what());
    }

    return 0;
}
