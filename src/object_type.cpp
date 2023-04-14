#include "value.h"
#include "khash.h"

KHASH_MAP_INIT_STR(SV, Value)

ObjectType::ObjectType() {
    hash = kh_init(SV);
}
ObjectType::ObjectType(ObjectType&& from) {
    hash = from.hash;
    from.hash = nullptr;
}
ObjectType& ObjectType::operator=(ObjectType&& from) {
    hash = from.hash;
    from.hash = nullptr;
    return *this;
}
ObjectType::~ObjectType() {
    if (hash) {
        kh_destroy(SV, hash);
    }
}

void ObjectType::set(const char* key, Value val) {
    auto ret = 0;
    auto n = kh_put(SV, hash, key, &ret);
    kh_val(hash, n) = val;
}
Value ObjectType::get(const char* key, bool& found) {
    auto n = kh_get(SV, hash, key);
    if (n == kh_end(hash)) {
        found = false;
        return value_null();
    }
    return kh_val(hash, n);
}

ObjectType::iter ObjectType::begin() {
    auto i = kh_begin(hash);
    while (!kh_exist(hash, i) && i != end()) {
        i++;
    }
    return i;
    // for (k = kh_begin(h); k != kh_end(h); ++k)  // traverse
    // if (kh_exist(h, k))            // test if a bucket contains data
    // kh_value(h, k) = 1;
    // kh_destroy(m32, h);              // deallocate the hash table
}
ObjectType::iter ObjectType::next(ObjectType::iter i) {
    i++;
    while (!kh_exist(hash, i) && i != end()) {
        i++;
    }
    return i;
}
const char* ObjectType::key(ObjectType::iter i) {
    return kh_key(hash, i);
}
Value ObjectType::value(ObjectType::iter i) {
    return kh_value(hash, i);
}
ObjectType::iter ObjectType::end() {
    return kh_end(hash);
}