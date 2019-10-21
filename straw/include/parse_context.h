#ifndef _PARSE_CONTEXT_H
#define _PARSE_CONTEXT_H

#include <string>

struct parse_context {
    std::string data;
    size_t pos = 0;
    size_t length = 0;

    parse_context() = default;
    parse_context(const parse_context&) = default;
    parse_context(parse_context&&) = default;
    parse_context(const std::string& s) {
        data = s;
        length = s.length();
        pos = 0;
    }
    parse_context& operator=(const parse_context&) = default;
    parse_context& operator=(parse_context&&) = default;

    // how many characters left?
    int unread() const { return length - pos; }
    
    // get the next character but don't advance
    char peek() const { return unread() ? data[pos] : 0; }
    
    // get the next n characters but don't advance
    string get(int n) const { return data.substr(pos, n); }
    
    // read the next character, or 0 if finished
    char read() { return unread() ? data[pos++] : 0; }
    
    // read n characters (stops at end)
    void read(size_t n) {
        pos += std::min(n, length - pos);
    }
    
    // read whitespace characters; return the number of characters read
    int read_space() {
        const auto spos = pos;
        for (; pos < data.length() && isspace(data[pos]); pos++);
        return pos - spos;
    }
    
    // return whether s equals the next part of the data (and consume |s| characters)
    bool read_if(const std::string& s) {
        if (unread() < s.length()) {
            return false;
        }
        if (strncmp(&data[pos], s.c_str(), s.length()) == 0) {
            pos += s.length();
            return true;
        }
        return false;
    }
    bool read_if(char c) {
        if (unread() && peek() == c) {
            pos += 1;
            return true;
        }
        return false;
    }
    std::string read_until(char c) {
        auto spos = pos;
        while (unread() && peek() != c) {
            pos++;
        }
        return data.substr(spos, pos - spos);
    }
    std::string read_until_ws() {
        auto spos = pos;
        while (unread() && !isspace(peek())) {
            pos++;
        }
        return data.substr(spos, pos - spos);
    }
    
    // read a number using strtod; returns true if successful
    bool read_number(double& result) {
        auto orig_pos = pos;
        read_space(); // important
        auto end = (char*)nullptr;
        result = strtod(&data[pos], &end);
        if (end == &data[pos]) {
            // conversion failed
            pos = orig_pos;
            return false;
        }
        pos += (size_t)(end - &data[pos]); // TODO: questionable?
        return true;
    }
};


#endif