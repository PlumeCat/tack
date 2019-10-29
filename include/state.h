#ifndef _STATE_H
#define _STATE_H

struct value;
ostream& operator<< (ostream& o, const value& val);

struct value {
    enum {
        NUMBER,
        STRING,
        LIST,
        FUNCTION,
    } type;

    struct function { int id; };

    // union {
        double dval;
        string sval;
        vector<value> lval;
        function fval;
    // };

    value() {
        type = NUMBER;
        dval = 0;
    }
    value(double val) {
        type = NUMBER;
        dval = val;
    }
    value(const string& s) {
        type = STRING;
        sval = s;
    }
    value(const vector<value>& l) {
        type = LIST;
        lval = l;
    }
    value(const function& f) {
        type = FUNCTION;
        fval = f;
    }
    value(const value&) = default;
    ~value() = default;
    value& operator=(const value& v) {
        type = v.type;
        switch (type) {
            case NUMBER:    dval = v.dval; break;
            case STRING:    sval = v.sval; break;
            case LIST:      lval = v.lval; break;
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
    value& operator=(const string& val) {
        sval = val;
        type = STRING;
        return *this;
    }
    value& operator=(const vector<value>& l) {
        type = LIST;
        lval = l;
        return *this;
    }
    value& operator=(const function& f) {
        fval = f;
        type = FUNCTION;
        return *this;
    }
    operator string() {
        stringstream s;
        s << *this;
        return s.str();
    }
};
ostream& operator<< (ostream& o, const value& val) {
    switch (val.type) {
        case value::NUMBER:     return o << val.dval;
        case value::STRING:     return o << val.sval;
        case value::LIST:       return o << val.lval;
        case value::FUNCTION:   return o << "function: " << val.fval.id;
        default:                return o << "<unknown-type>";
    }
}

using scope = map<string, value>;
struct state {
    vector<scope> scopes;
    vector<ast> functions; // interned functions

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
    void set_local(const string& name, const value& val) {
        scopes[scopes.size()-1][name] = val;
    }
    value& get_local(const string& name, int frame = -1) {
        if (frame == -1) {
            frame = scopes.size() - 1;
        }
        auto& s = scopes[frame];
        auto f = s.find(name);
        if (f == s.end()) {
            if (frame == 0) {
                throw runtime_error("Unknown variable: " + name);
            } else {
                return get_local(name, frame - 1);
            }
        }
        return f->second;
    }
};
#endif
