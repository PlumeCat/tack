#include "../include/tack.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <unordered_map>
#include <chrono>
#include <filesystem>

#if (defined _MSC_VER && defined _DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

int main(int argc, char* argv[]) {
    auto vm = TackVM::create();
    vm->add_module_dir();
    vm->add_libs();

    auto files = std::vector<std::string>{};
    for (auto i = 1; i < argc; i++) {
        files.emplace_back(argv[i]);
    }

    std::ios::sync_with_stdio(false);
    if (!files.size()) {
        std::cout << "error: no source files provided (repl not supported yet)" << std::endl;
        return 1;
    }

    try {
        for (auto& f: files) {
            vm->load_module(f);
        }
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
