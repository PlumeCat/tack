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

using namespace std;
using namespace std::chrono;

#include "parsing.h"
#include "value.h"
// #include "compiler.h"
// #include "interpreter.h"

#include "compiler2.h"
#include "interpreter2.h"


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
    auto fname = (argc >= 2) ? argv[1] : "source.str";
    auto s = read_text_file(fname);
    if (!s.has_value()) {
        throw runtime_error("error opening source file: "s + argv[1]);
    }
    auto source = s.value();
    auto out_ast = AstNode {};
    try {
        parse(source, out_ast);
        if (check_arg("-A")) {
            log("AST: ", out_ast.tostring());
        }
        
        auto program = Program {};
        auto compiler = Compiler2{};
        auto vm = Interpreter2();
        compiler.compile(out_ast, program);
        if (check_arg("-D")) {
            log(program.to_string());
        }

        //auto before = steady_clock::now();
        vm.execute(program);
        //auto after = steady_clock::now();
        //log("BC done in ", duration_cast<microseconds>(after - before).count() * 1e-6, "s");
    } catch (exception& e) {
        log(e.what());
    }

    return 0;
}
