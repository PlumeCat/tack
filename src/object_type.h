#pragma once

#include <vector>
#include <string>

// HACK:
struct kh_SV_s;

struct ObjectType {
    kh_SV_s* hash; // HACK:
    using iter = unsigned int; // HACK:
    
    ObjectType();
    ObjectType(const ObjectType&) = delete;
    ObjectType& operator=(const ObjectType&) = delete;
    ObjectType(ObjectType&& from);
    ObjectType& operator=(ObjectType&& from);
    ~ObjectType();

    void set(const char* key, Value val);
    Value get(const char* key, bool& found);
    iter begin();
    iter end();
    iter next(iter i);
    
    const char* key(iter i);
    Value value(iter i);

};

