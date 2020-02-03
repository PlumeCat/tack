#ifndef _LOGGING_H
#define _LOGGING_H

struct _log {
    stringstream null_log = stringstream();

    ostream& out() {
        return null_log;
        return cerr;
    }

    ostream& debug() {
        return out() << "DEBUG: ";
    }
    ostream& info() {
        return out() << "INFO: ";
    }
    ostream& error() {
        return cerr << "ERROR: ";
    }
} logger;

struct _logfunc {
    string func;
    _logfunc(const string& f) {
        func = f;
        logger.debug() << " -- entering " << func << endl;
    }
    ~_logfunc() {
        logger.debug() << " -- exiting " << func << endl;
    }
};
#define log_func() auto _logfunc_##__LINE__ = _logfunc(string(__FUNCTION__))

#endif
