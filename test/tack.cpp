#include "../src/interpreter.h"
#include "../src/library.h"

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

int main(int argc, char* argv[]) {
    auto vm = Interpreter {};
    setup_standard_library(&vm);

    auto files = std::vector<std::string>{};
    for (auto i = 1; i < argc; i++) {
        auto arg = std::string(argv[i]);
        if (arg == "-A") {
            vm.log_ast = true;
        } else if  (arg == "-D") {
            vm.log_bytecode = true;
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
        for (auto& f: files) {
            vm.load_module(f);
        }
    } catch (std::exception& e) {
        log(e.what());
    }

    return 0;
}
