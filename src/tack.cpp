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


int main(int argc, char* argv[]) {
    auto check_arg = [&](const std::string& s) {
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
        throw std::runtime_error("error opening source file: " + std::string(argv[1]));
    }
    auto& source = s.value();
    auto out_ast = AstNode {};
    try {
        parse(source, out_ast);
        if (check_arg("-A")) {
            log("AST: ", out_ast.tostring());
        }
        
        auto global = AstNode(AstType::FuncLiteral, AstNode(AstType::ParamDef), out_ast);
        auto compiler = Compiler { .node = &global, .name = "(global)" };
        auto program = CodeFragment {};
        compiler.compile_func(&global, &program);

        auto vm = Interpreter{};
        if (check_arg("-D")) {
            log("Program:\n" + program.str());
        }

        vm.execute(&program);
    } catch (std::exception& e) {
        log(e.what());
    }

    return 0;
}
