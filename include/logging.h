#ifndef _LOGGING_H
#define _LOGGING_H

struct _log {
    stringstream null = stringstream();

    ostream& out() {
        return null;
        return cerr;
    }

    ostream& debug() {
        return out() << "DEBUG: ";
    }
    ostream& info() {
        return out() << "INFO: ";
    }
    ostream& error() {
        return out() << "ERROR: ";
    }
} log;

struct _logfunc {
    string func;
    _logfunc(const string& f) {
        func = f;
        log.debug() << " -- entering " << func << endl;
    }
    ~_logfunc() {
        log.debug() << " -- exiting " << func << endl;
    }
};
#define log_func() auto _logfunc_##__LINE__ = _logfunc(string(__FUNCTION__))

#endif
