#ifndef _UTIL_H
#define _UTIL_H

#include <string>
#include <cctype>

#include "definitions.h"

template<typename t>
ostream& operator<<(ostream& o, const vector<t>& c) {
    o << "[";
    if (c.size()) {
        o << " " << c[0];
        for (auto e = c.begin()+1; e != c.end(); e++) {
            o << ", " << (*e);
        }
        o << " ";
    }
    return o << "]";
}

// trim leading whitespace; return the resulting string
string trim_l(const string& d) {
    auto p = 0;
    auto l = d.length();
    for (; p < l && isspace(d[p]); p++);
    return d.substr(p, l - p);
}

// trim trailing whitespace; return the resulting string
string trim_r(const string& d) {
    auto n = d.length();
    for (; n > 0 && isspace(d[n]); n--);
    return d.substr(0, n);
}

// read contents of file into a string, return results;
string read_text_file(const string& fname) {
    auto file = ifstream(fname);
    if (!file.is_open()) {
        return "";
    }

    // get file size
    file.seekg(0, ios_base::end);
    const auto file_size = file.tellg();
    file.seekg(0, ios_base::beg);

    auto data = string(file_size, 0);
    file.read(&data[0], file_size);
    return data;
}


#endif
