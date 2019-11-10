#ifndef _STATE_H
#define _STATE_H

struct value;
ostream& operator << (ostream&, const value&);
using native_function = value (*)(const vector<value>&);
using int64 = int64_t;

struct value {
    using function  = struct { size_t id; };
    using itype     = struct { size_t id; };
    using list      = vector<value>;
    using object    = map<string, value>;

    enum {
        NUMBER,
        STRING,
        LIST,
        TYPE,
        OBJECT,
        FUNCTION,
        NATIVE_FUNC,
    } type;

    // union {
    double      dval;
    string      sval;
    list        lval;
    object      oval;
    function    fval;
    itype       tval;
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
    value(const list& l) {
        type = LIST;
        lval = l;
    }
    value(const object& o) {
        type = OBJECT;
        oval = o;
    }
    value(const function& f) {
        type = FUNCTION;
        fval = f;
    }
    value(const itype& t) {
        type = TYPE;
        tval = t;
    }

    value(const value& v) {
        type = v.type;
        switch (type) {
            case NUMBER:    dval = v.dval; break;
            case STRING:    sval = v.sval; break;
            case LIST:      lval = v.lval; break;
            case OBJECT:    oval = v.oval; break;
            case FUNCTION:  fval = v.fval; break;
            case TYPE:      tval = v.tval; break;
            default:        throw runtime_error("unknown type!");
        }
    }
    ~value() = default;

    value& operator=(const value& v) {
        type = v.type;
        switch (type) {
            case NUMBER:    dval = v.dval; break;
            case STRING:    sval = v.sval; break;
            case LIST:      lval = v.lval; break;
            case OBJECT:    oval = v.oval; break;
            case FUNCTION:  fval = v.fval; break;
            case TYPE:      tval = v.tval; break;
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
    value& operator=(const list& l) {
        type = LIST;
        lval = l;
        return *this;
    }
    value& operator=(const object& o) {
        type = OBJECT;
        oval = o;
        return *this;
    }
    value& operator=(const function& f) {
        fval = f;
        type = FUNCTION;
        return *this;
    }
    value& operator=(const itype& t) {
        type = TYPE;
        tval = t;
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
        case value::OBJECT:     return o << val.oval;
        case value::TYPE:       return o << "type: " << val.tval.id;
        case value::FUNCTION:   return o << "function: " << val.fval.id;
        default:                return o << "<unknown-type>";
    }
}


struct state {
    using scope = map<string, value>;

    vector<scope> scopes;
    vector<ast> functions;    // interned functions
    vector<ast> types;        // interned types

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
    void define_local(const string& name, const value& val) {
        scopes[scopes.size()-1][name] = val;
    }
    void set_local(const string& name, const value& val) {
        get_local(name) = val;
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
