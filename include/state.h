#ifndef _STATE_H
#define _STATE_H

struct state {
    vector<map<string, double>> scopes;

    state() {
        // create the global scope
        push_scope();
    }
    state(const state&) = default;
    ~state() = default;
    state& operator=(const state&) = default;

    // entering and exiting scopes
    void push_scope() {
        scopes.push_back(map<string, double>());        
    }
    void pop_scope() {
        scopes.pop_back();
    }

    // variables
    void local(const string& name, double val) {
        scopes[scopes.size()-1][name] = val;
    }
    double local(const string& name) {
        return scopes[scopes.size()-1][name];
    }
};

#endif