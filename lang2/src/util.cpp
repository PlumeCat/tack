#include <fstream>

#include "util.h"

using namespace std;

string read_text_file(const string& fname) {
    auto f = ifstream(fname);
    if (!f.is_open()) {
        return ""s;
    }

    // get file size
    f.seekg(0, ios::end);
    auto size = f.tellg();
    f.seekg(0, ios::beg);

    // read entire file
    auto s = string(size, '~');
    f.read(s.data(), size);
    f.close();

    return s;
}