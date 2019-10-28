#include <iostream>
#include <fstream>
#include <exception>
#include <optional>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <array>
#include <numeric>
#include <string_view>
#include <algorithm>

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <ctime>

using namespace std;

#include "logging.h"
#include "util.h"
//#include "parse_context.h"
#include "ast.h"
#include "parsing.h"
// #include "heapsort.h"
#include "state.h"
#include "eval.h"

void execute(const string& source) {
    auto ptr = (char*)source.c_str();
    auto program = ast();
    auto vmstate = state();

    try {
        parse(ptr, program);
        log.debug() << program << endl;
        eval(program, vmstate);
    } catch (parse_end&) {
        log.error() << "unexpected end of file" << endl;
    } catch (invalid_program&) {
        log.error() << "invalid program" << endl;
    } catch (exception& e) {
        log.error() << "runtime error: " << e.what() << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        // repl mode
        // not very good, can't handle multi lines yet
        auto line = string();
        while (true) {
            if (cin.eof()) {
                break;
            }

            cout << "> " << flush;
            getline(cin, line);
            if (!line.length()) {
                continue;
            }
            if (line == "exit") {
                break;
            }
            execute(line);
        }
    } else {
        // source file mode
        auto fname = argv[1];
        log.info() << "Loading: " << fname << endl;
        auto data = read_text_file(fname);
        log.info() << "length: " << data.length() << " | " << strlen(&*data.begin()) << endl;
        execute(data);
    }

    return 0;
}

/*
TODO:
    function parameters
    indirect functions instead of by value
    if else
    for / while / map / ...
    lists, strings
    comments

    parsing improvements:
        - unify macro approach (or define grammar combinators), remove hardcoded bits
        - specify ast schema, add validator
        - very inefficient, just inspect "log_func" for all parsers. improve?
        - can't handle single-argument functions
        - can't handle chained postfix operators  eg hof()(), ll[1][2]
        - find a good way of checking exhaustively for ambiguities (and even for resolving them)

*/
