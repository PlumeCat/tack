#ifndef _LOGGING_H
#define _LOGGING_H

struct _logfunc {
    string func;
    _logfunc(const string& f) {
        func = f;
        cerr << "Entering " << func << endl;
    }
    ~_logfunc() {
        cerr << "Exiting " << func << endl;
    }
};
#define log_func() auto _logfunc_##__LINE__ = _logfunc(string(__FUNCTION__) + "{" + string(ptr) + "}")

#endif