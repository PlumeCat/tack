#ifndef _STATE_H
#define _STATE_H
struct value {
    enum {
        NUMBER,
        FUNCTION,
    } type;
    // union {
        double dval;
        ast fval;
    // };
    value() {
        type = NUMBER;
        dval = 0;
    }
    value(double val) {
        type = NUMBER;
        dval = val;
    }
    value(const ast& a) {
        type = FUNCTION;
        fval = a;
    }
    value(const value&) = default;
    ~value() = default;
    value& operator=(const value& v) {
        type = v.type;
        switch (type) {
            case NUMBER:    dval = v.dval; break;
            case FUNCTION:  fval = v.fval; break;
            default:        throw runtime_error("Unknown type!");
        }
        return *this;
    }
    value& operator=(double val) {
        dval = val;
        type = NUMBER;
        return *this;
    }
    value& operator=(const ast& body) {
        fval = body;
        type = FUNCTION;
        return *this;
    }
};
ostream& operator<< (ostream& o, const value& val) {
    switch (val.type) {
        case value::NUMBER:     return o << val.dval;
        case value::FUNCTION:   return o << val.fval;
        default:                return o << "<unknown-type>";
    }
}
using scope = map<string, value>;
struct state {
    vector<scope> scopes;
    state() = default;
    state(const state&) = default;
    ~state() = default;
    state& operator=(const state&) = default;
    // entering and exiting scopes
    void push_scope() {
        scopes.push_back(scope());
    }
    void pop_scope() {
        scopes.pop_back();
    }
    // variables
    void local(const string& name, const value& val) {
        scopes[scopes.size()-1][name] = val;
    }
    value& local(const string& name) {
        auto& s = scopes[scopes.size()-1];
        auto f = s.find(name);
        if (f == s.end()) {
            throw runtime_error("Unknown local variable: " + name);
        }
        return f->second;
    }
};
#endif
