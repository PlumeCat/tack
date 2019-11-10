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
#include <chrono>
#include <thread>
#include <cstdint>

#include <cmath>
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

value tack_sqrt(const vector<value>& args) {
    return value(sqrt(args[0].dval));
}

value tack_print(const vector<value>& args) {
    for (const auto& a: args) {
        cout << a << " ";
    }
    cout << endl;
    return value(0);
}

void execute(const string& source) {
    auto ptr = (char*)source.c_str();
    auto program = ast();
    auto vmstate = state();

    // invisible root scope for builtin functions
    vmstate.push_scope();
    vmstate.define_local("print", define_function(tack_print, vmstate));
    vmstate.define_local("sqrt",  define_function(tack_sqrt,  vmstate));

    try {
        logger.debug() << "parsing program" << endl;
        parse(ptr, program);
        logger.debug() << program << endl;
        eval(program, vmstate);
    } catch (parse_end&) {
        logger.error() << "unexpected end of file" << endl;
    } catch (invalid_program&) {
        logger.error() << "invalid program" << endl;
    } catch (exception& e) {
        logger.error() << "runtime error: " << e.what() << endl;
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
        logger.info() << "Loading: " << fname << endl;
        auto data = read_text_file(fname);
        logger.info() << "length: " << data.length() << " | " << strlen(&*data.begin()) << endl;
        execute(data);
    }

    return 0;
}
