#include <iostream>
#include <vector>
#include <fstream>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <chrono>

using namespace std;
using namespace std::chrono;

#include "util.h"
#include "parse.h"
#include "eval.h"

int func(int a, int b) {


}

int main(int argc, char* argv[]) {
    auto source = read_text_file(argv[1]);

    auto source_file = Ast {};
    auto before = steady_clock::now();
    auto source_view = string_view(source);
    auto success = parse_source_file(source_view, source_file);
    auto after = steady_clock::now();

    if (success) {
        print_ast(source_file);
        auto vm = VM {};
        try {
            eval_module(source_file, vm);
            auto call_main = Ast { Ast::CallExpr, "main", 0.0, { { Ast::ArgsList, ""s, 0.0, {} }} };
            eval_call_expr(call_main, vm);
        } catch (exception& e) {
            cout << "runtime error: " << e.what() << endl;
        }
    } else {
        cout << "parse failed" << endl;
    }

    cout << "parse time: " << duration_cast<microseconds>(after - before).count() << " us" << endl;
    
    return 0;
}