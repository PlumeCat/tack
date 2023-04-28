#pragma once

#include <vector>
#include <string>

// HACK:
// struct kh_SV_s;
// struct ObjectHash;
#include "khash2.h"

struct ObjectType {
    // kh_SV_s* hash; // HACK:
    KHash<std::string, Value> hash;

    uint32_t len = 0;
    GCType()
    using iter = unsigned int; // HACK:
    
    ObjectType();
    ObjectType(const ObjectType&) = delete;
    ObjectType& operator=(const ObjectType&) = delete;
    ObjectType(ObjectType&& from) = delete;
    ObjectType& operator=(ObjectType&& from) = delete;
    ~ObjectType();

    void set(const std::string& key, Value val);
    Value get(const std::string& key, bool& found);
    iter begin();
    iter end();
    iter next(iter i);
    uint32_t length();
    
    const std::string& key(iter i);
    Value value(iter i);
};

